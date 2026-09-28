pipeline {
    agent any

    environment {
        IMAGE_NAME = "network-monitor"
        REGISTRY = "localhost:5000"
        REGISTRY_IMAGE = "${REGISTRY}/${IMAGE_NAME}"

        CONTAINER_NAME = "network-monitor-${BUILD_NUMBER}"
        HOST_PORT = "19000"

        RETAIN_BUILDS = "2"
    }

    stages {

        stage('Checkout') {
            steps {
                checkout scm
            }
        }

        stage('Build C++ Application') {
            steps {
                sh '''
                    set -e

                    rm -rf build

                    cmake -S . -B build
                    cmake --build build -j$(nproc)
                '''
            }
        }

        stage('Unit Tests') {
            steps {
                sh '''
                    set -e

                    cd build
                    ctest --output-on-failure
                '''
            }
        }

        stage('Native API Tests') {
            steps {
                sh '''
                    set -e

                    ./build/network-monitor > native-server.log 2>&1 &
                    APP_PID=$!

                    cleanup() {
                        kill $APP_PID 2>/dev/null || true
                    }

                    trap cleanup EXIT

                    echo "Waiting for application..."

                    for i in $(seq 1 20); do
                        if curl -fsS http://127.0.0.1:9000/health > /dev/null; then
                            echo "Application is ready."
                            break
                        fi

                        sleep 1
                    done

                    echo "Testing /health"
                    curl -fsS http://127.0.0.1:9000/health
                    echo

                    echo "Testing /system"
                    curl -fsS http://127.0.0.1:9000/system
                    echo

                    echo "Testing /stats"
                    curl -fsS http://127.0.0.1:9000/stats
                    echo
                '''
            }
        }

        stage('Build Docker Image') {
            steps {
                script {
                    env.BUILD_IMAGE_TAG =
                        "${REGISTRY_IMAGE}:build-${BUILD_NUMBER}"

                    sh """
                        set -e

                        echo "Building Docker image:"
                        echo "${BUILD_IMAGE_TAG}"

                        docker build \
                            -t ${BUILD_IMAGE_TAG} \
                            .
                    """
                }
            }
        }

        stage('Push Docker Image') {
            steps {
                sh """
                    set -e

                    echo "Pushing ${BUILD_IMAGE_TAG}"

                    docker push ${BUILD_IMAGE_TAG}
                """
            }
        }

        stage('Registry Retention - Keep Last 2') {
            steps {
                sh '''
                    set -e

                    echo "======================================"
                    echo "Registry retention policy"
                    echo "Keeping latest 2 build images"
                    echo "======================================"

                    TAGS=$(curl -fsS \
                        http://localhost:5000/v2/network-monitor/tags/list)

                    echo "All registry tags:"
                    echo "$TAGS"

                    echo
                    echo "Build tags before cleanup:"

                    echo "$TAGS" | python3 -c '
import sys
import json

data = json.load(sys.stdin)

tags = data.get("tags", [])

build_tags = []

for tag in tags:
    if tag.startswith("build-"):
        try:
            number = int(tag.split("-", 1)[1])
            build_tags.append((number, tag))
        except ValueError:
            pass

for number, tag in sorted(build_tags, reverse=True):
    print(tag)
'

                    echo
                    echo "Determining old builds..."

                    OLD_TAGS=$(echo "$TAGS" | python3 -c '
import sys
import json

data = json.load(sys.stdin)

tags = data.get("tags", [])

build_tags = []

for tag in tags:
    if tag.startswith("build-"):
        try:
            number = int(tag.split("-", 1)[1])
            build_tags.append((number, tag))
        except ValueError:
            pass

build_tags.sort(reverse=True)

keep = build_tags[:2]
delete = build_tags[2:]

print("KEEP:")
for number, tag in keep:
    print(tag, file=sys.stderr)

print("DELETE:", file=sys.stderr)
for number, tag in delete:
    print(tag)
')

                    if [ -z "$OLD_TAGS" ]; then
                        echo
                        echo "No old build images to delete."
                    else
                        echo
                        echo "Deleting old build images..."

                        for TAG in $OLD_TAGS; do

                            echo
                            echo "Processing $TAG"

                            DIGEST=$(curl -fsSI \
                                -H "Accept: application/vnd.docker.distribution.manifest.v2+json" \
                                "http://localhost:5000/v2/network-monitor/manifests/$TAG" \
                                | awk -F': ' 'tolower($1)=="docker-content-digest" {print $2}' \
                                | tr -d '\\r')

                            if [ -z "$DIGEST" ]; then
                                echo "Could not find digest for $TAG"
                                exit 1
                            fi

                            echo "Digest: $DIGEST"

                            echo "Deleting manifest..."

                            curl -fsS -X DELETE \
                                "http://localhost:5000/v2/network-monitor/manifests/$DIGEST"

                            echo
                            echo "Deleted $TAG"
                        done
                    fi

                    echo
                    echo "======================================"
                    echo "Registry tags after cleanup"
                    echo "======================================"

                    curl -fsS \
                        http://localhost:5000/v2/network-monitor/tags/list

                    echo
                '''
            }
        }

        stage('Deploy Container') {
            steps {
                sh """
                    set -e

                    echo "Removing previous container if present..."

                    docker rm -f ${CONTAINER_NAME} 2>/dev/null || true

                    echo "Pulling current build..."

                    docker pull ${BUILD_IMAGE_TAG}

                    echo "Starting container..."

                    docker run -d \
                        --name ${CONTAINER_NAME} \
                        -p ${HOST_PORT}:9000 \
                        ${BUILD_IMAGE_TAG}

                    docker ps --filter "name=${CONTAINER_NAME}"
                """
            }
        }

        stage('Container API Tests') {
            steps {
                sh '''
                    set -e

                    echo "Waiting for Docker container..."

                    for i in $(seq 1 20); do

                        if curl -fsS \
                            http://127.0.0.1:${HOST_PORT}/health > /dev/null; then

                            echo "Container is ready."
                            break
                        fi

                        sleep 1
                    done

                    echo
                    echo "Testing container /health"

                    curl -fsS \
                        http://127.0.0.1:${HOST_PORT}/health

                    echo

                    echo "Testing container /system"

                    curl -fsS \
                        http://127.0.0.1:${HOST_PORT}/system

                    echo

                    echo "Testing container /stats"

                    curl -fsS \
                        http://127.0.0.1:${HOST_PORT}/stats

                    echo
                '''
            }
        }
    }

    post {

        success {
            echo """
========================================
CI/CD PIPELINE SUCCESS
========================================

Jenkins build: ${BUILD_NUMBER}

Docker image:
${BUILD_IMAGE_TAG}

Registry retention:
Latest 2 builds only

========================================
"""
        }

        failure {
            echo """
========================================
CI/CD PIPELINE FAILED
========================================

Jenkins build: ${BUILD_NUMBER}

========================================
"""
        }

        always {
            sh '''
                docker rm -f ${CONTAINER_NAME} 2>/dev/null || true
            '''
        }
    }
}
