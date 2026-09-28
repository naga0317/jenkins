pipeline {
    agent any

    environment {
        APP_VERSION = "1.0.0"
        IMAGE_NAME = "network-monitor"
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
                script {

                    def gitCommit = sh(
                        script: 'git rev-parse --short HEAD',
                        returnStdout: true
                    ).trim()

                    env.VERSION_TAG = "${IMAGE_NAME}:${APP_VERSION}"
                    env.GIT_IMAGE_TAG = "${IMAGE_NAME}:git-${gitCommit}"
                    env.BUILD_IMAGE_TAG = "${IMAGE_NAME}:build-${BUILD_NUMBER}"

                    sh """
                        set -e

                        echo "========================================"
                        echo "Building Docker images"
                        echo "========================================"

                        echo "Version tag : ${VERSION_TAG}"
                        echo "Git tag     : ${GIT_IMAGE_TAG}"
                        echo "Build tag   : ${BUILD_IMAGE_TAG}"

                        docker build \
                            -t ${VERSION_TAG} \
                            -t ${GIT_IMAGE_TAG} \
                            -t ${BUILD_IMAGE_TAG} \
                            .

                        echo "========================================"
                        echo "Docker images created"
                        echo "========================================"

                        docker images ${IMAGE_NAME}
                    """
                }
            }
        }

        stage('Deploy Container') {
            steps {
                sh '''
                    set -e

                    echo "Removing previous test container..."

                    docker rm -f ${CONTAINER_NAME} 2>/dev/null || true

                    echo "Starting container..."

                    docker run -d \
                        --name ${CONTAINER_NAME} \
                        -p ${HOST_PORT}:9000 \
                        ${VERSION_TAG}

                    echo "Container started:"
                    docker ps --filter "name=${CONTAINER_NAME}"
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
                            echo "Container is ready."
                            break
                        fi

                        sleep 1

                    done

                    echo "========================================"
                    echo "Testing container /health"
                    echo "========================================"

                    curl -fsS http://127.0.0.1:${HOST_PORT}/health

                    echo

                    echo "========================================"
                    echo "Testing container /system"
                    echo "========================================"

                    curl -fsS http://127.0.0.1:${HOST_PORT}/system

                    echo

                    echo "========================================"
                    echo "Testing container /stats"
                    echo "========================================"

                    curl -fsS http://127.0.0.1:${HOST_PORT}/stats

                    echo

                    echo "========================================"
                    echo "Container API tests passed"
                    echo "========================================"
                '''
            }
        }
    }

    post {

        success {
            echo "========================================"
            echo "CI/CD PIPELINE SUCCESS"
            echo "========================================"
            echo "Application version: ${APP_VERSION}"
            echo "Docker image: ${VERSION_TAG}"
            echo "Git image: ${GIT_IMAGE_TAG}"
            echo "Build image: ${BUILD_IMAGE_TAG}"
        }

        failure {
            echo "========================================"
            echo "CI/CD PIPELINE FAILED"
            echo "========================================"
        }

        always {
            sh '''
                echo "Cleaning up test container..."

                docker rm -f ${CONTAINER_NAME} 2>/dev/null || true
            '''
        }
    }
}
