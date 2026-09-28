pipeline {
    agent any

    environment {
        IMAGE_NAME = "network-monitor:${BUILD_NUMBER}"
        CONTAINER_NAME = "network-monitor-${BUILD_NUMBER}"
        HOST_PORT = "19000"
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
                            break
                        fi
                        sleep 1
                    done

                    echo "Testing /health"
                    curl -fsS http://127.0.0.1:9000/health

                    echo "Testing /system"
                    curl -fsS http://127.0.0.1:9000/system

                    echo "Testing /stats"
                    curl -fsS http://127.0.0.1:9000/stats
                '''
            }
        }

        stage('Build Docker Image') {
            steps {
                sh '''
                    set -e

                    docker build \
                        -t ${IMAGE_NAME} \
                        .
                '''
            }
        }

        stage('Deploy Container') {
            steps {
                sh '''
                    set -e

                    docker rm -f ${CONTAINER_NAME} 2>/dev/null || true

                    docker run -d \
                        --name ${CONTAINER_NAME} \
                        -p ${HOST_PORT}:9000 \
                        ${IMAGE_NAME}
                '''
            }
        }

        stage('Container API Tests') {
            steps {
                sh '''
                    set -e

                    echo "Waiting for Docker container..."

                    for i in $(seq 1 20); do
                        if curl -fsS http://127.0.0.1:${HOST_PORT}/health > /dev/null; then
                            break
                        fi
                        sleep 1
                    done

                    echo "Testing container /health"
                    curl -fsS http://127.0.0.1:${HOST_PORT}/health

                    echo "Testing container /system"
                    curl -fsS http://127.0.0.1:${HOST_PORT}/system

                    echo "Testing container /stats"
                    curl -fsS http://127.0.0.1:${HOST_PORT}/stats
                '''
            }
        }
    }

    post {
        always {
            sh '''
                docker rm -f ${CONTAINER_NAME} 2>/dev/null || true
            '''
        }
    }
}
