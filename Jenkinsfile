pipeline {
    agent any

    options {
        timestamps()
        disableConcurrentBuilds()
    }

    triggers {
        githubPush()
    }

    environment {
        IMAGE_NAME = 'network-monitor'
    }

    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }

        stage('Build Docker image') {
            steps {
                sh 'docker build --tag "$IMAGE_NAME:$BUILD_NUMBER" .'
            }
        }

        stage('Container smoke test') {
            steps {
                sh '''
                    set -eu

                    image="$IMAGE_NAME:$BUILD_NUMBER"
                    container="network-monitor"

                    cleanup() {
                        docker rm -f "$container" >/dev/null 2>&1 || true
                    }
                    trap cleanup EXIT

                    docker run --detach \
                        --name "$container" \
                        --publish 127.0.0.1::9000 \
                        "$image" >/dev/null

                    port=$(docker port "$container" 9000/tcp | sed 's/.*://')
                    ready=0

                    for attempt in $(seq 1 30); do
                        if health=$(curl --fail --silent "http://127.0.0.1:$port/health"); then
                            ready=1
                            break
                        fi
                        sleep 1
                    done

                    if [ "$ready" -ne 1 ]; then
                        echo 'Container did not become healthy.'
                        docker logs "$container"
                        exit 1
                    fi

                    printf '%s' "$health" | grep -q '"status":"UP"'
                    curl --fail --silent "http://127.0.0.1:$port/system" >/dev/null
                    curl --fail --silent "http://127.0.0.1:$port/stats" >/dev/null
                '''
            }
        }
    }

    post {
        always {
            sh 'docker image rm "$IMAGE_NAME:$BUILD_NUMBER" >/dev/null 2>&1 || true'
        }
    }
}
