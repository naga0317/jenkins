pipeline {
    agent any

    environment {
        APP_VERSION = "1.0.0"

        IMAGE_NAME = "network-monitor"

        REGISTRY = "localhost:5000"

        REGISTRY_IMAGE = "${REGISTRY}/${IMAGE_NAME}"

        CONTAINER_NAME = "network-monitor-${BUILD_NUMBER}"

        HOST_PORT = "19000"
    }

    stages {

        /*
         * ==========================================
         * 1. CHECKOUT
         * ==========================================
         */

        stage('Checkout') {
            steps {
                checkout scm
            }
        }


        /*
         * ==========================================
         * 2. BUILD C++ APPLICATION
         * ==========================================
         */

        stage('Build C++ Application') {
            steps {
                sh '''
                    set -e

                    echo "========================================"
                    echo "Building C++ application"
                    echo "========================================"

                    rm -rf build

                    cmake -S . -B build

                    cmake --build build -j$(nproc)
                '''
            }
        }


        /*
         * ==========================================
         * 3. UNIT TESTS
         * ==========================================
         */

        stage('Unit Tests') {
            steps {
                sh '''
                    set -e

                    echo "========================================"
                    echo "Running unit tests"
                    echo "========================================"

                    cd build

                    ctest --output-on-failure
                '''
            }
        }


        /*
         * ==========================================
         * 4. NATIVE API TESTS
         * ==========================================
         */

        stage('Native API Tests') {
            steps {
                sh '''
                    set -e

                    echo "========================================"
                    echo "Starting native application"
                    echo "========================================"

                    ./build/network-monitor > native-server.log 2>&1 &

                    APP_PID=$!

                    cleanup() {
                        kill $APP_PID 2>/dev/null || true
                    }

                    trap cleanup EXIT

                    echo "Waiting for application..."

                    for i in $(seq 1 20); do

                        if curl -fsS \
                            http://127.0.0.1:9000/health \
                            > /dev/null; then

                            echo "Application is ready."

                            break
                        fi

                        sleep 1

                    done


                    echo "========================================"
                    echo "Testing /health"
                    echo "========================================"

                    curl -fsS \
                        http://127.0.0.1:9000/health

                    echo


                    echo "========================================"
                    echo "Testing /system"
                    echo "========================================"

                    curl -fsS \
                        http://127.0.0.1:9000/system

                    echo


                    echo "========================================"
                    echo "Testing /stats"
                    echo "========================================"

                    curl -fsS \
                        http://127.0.0.1:9000/stats

                    echo

                    echo "Native API tests passed."
                '''
            }
        }


        /*
         * ==========================================
         * 5. BUILD DOCKER IMAGE
         * ==========================================
         */

        stage('Build Docker Image') {
            steps {

                script {

                    def gitCommit = sh(
                        script: 'git rev-parse --short HEAD',
                        returnStdout: true
                    ).trim()

                    /*
                     * Save Git commit for later stages.
                     */

                    env.GIT_COMMIT_SHORT = gitCommit


                    /*
                     * Local image tags
                     */

                    env.VERSION_TAG =
                        "${IMAGE_NAME}:${APP_VERSION}"

                    env.GIT_IMAGE_TAG =
                        "${IMAGE_NAME}:git-${gitCommit}"

                    env.BUILD_IMAGE_TAG =
                        "${IMAGE_NAME}:build-${BUILD_NUMBER}"


                    /*
                     * Registry image tags
                     */

                    env.REGISTRY_VERSION_TAG =
                        "${REGISTRY_IMAGE}:${APP_VERSION}"

                    env.REGISTRY_GIT_TAG =
                        "${REGISTRY_IMAGE}:git-${gitCommit}"

                    env.REGISTRY_BUILD_TAG =
                        "${REGISTRY_IMAGE}:build-${BUILD_NUMBER}"


                    sh """
                        set -e

                        echo "========================================"
                        echo "Building Docker image"
                        echo "========================================"

                        echo "Application version : ${APP_VERSION}"
                        echo "Git commit         : ${gitCommit}"
                        echo "Jenkins build      : ${BUILD_NUMBER}"

                        echo

                        echo "Local tags:"
                        echo "  ${VERSION_TAG}"
                        echo "  ${GIT_IMAGE_TAG}"
                        echo "  ${BUILD_IMAGE_TAG}"

                        docker build \
                            -t ${VERSION_TAG} \
                            -t ${GIT_IMAGE_TAG} \
                            -t ${BUILD_IMAGE_TAG} \
                            .

                        echo

                        echo "Docker build completed."

                        docker images ${IMAGE_NAME}
                    """
                }
            }
        }


        /*
         * ==========================================
         * 6. TAG FOR LOCAL REGISTRY
         * ==========================================
         */

        stage('Tag Registry Images') {
            steps {

                sh """
                    set -e

                    echo "========================================"
                    echo "Tagging images for local registry"
                    echo "========================================"

                    echo "Version:"
                    echo "  ${VERSION_TAG}"
                    echo "  -> ${REGISTRY_VERSION_TAG}"

                    docker tag \
                        ${VERSION_TAG} \
                        ${REGISTRY_VERSION_TAG}


                    echo "Git:"
                    echo "  ${GIT_IMAGE_TAG}"
                    echo "  -> ${REGISTRY_GIT_TAG}"

                    docker tag \
                        ${GIT_IMAGE_TAG} \
                        ${REGISTRY_GIT_TAG}


                    echo "Build:"
                    echo "  ${BUILD_IMAGE_TAG}"
                    echo "  -> ${REGISTRY_BUILD_TAG}"

                    docker tag \
                        ${BUILD_IMAGE_TAG} \
                        ${REGISTRY_BUILD_TAG}


                    echo

                    echo "Registry images:"

                    docker images ${REGISTRY_IMAGE}
                """
            }
        }


        /*
         * ==========================================
         * 7. PUSH TO LOCAL DOCKER REGISTRY
         * ==========================================
         */

        stage('Push Docker Images') {
            steps {

                sh """
                    set -e

                    echo "========================================"
                    echo "Pushing images to local registry"
                    echo "========================================"

                    echo "Registry:"
                    echo "  ${REGISTRY}"

                    echo

                    echo "Pushing:"
                    echo "  ${REGISTRY_VERSION_TAG}"

                    docker push \
                        ${REGISTRY_VERSION_TAG}


                    echo

                    echo "Pushing:"
                    echo "  ${REGISTRY_GIT_TAG}"

                    docker push \
                        ${REGISTRY_GIT_TAG}


                    echo

                    echo "Pushing:"
                    echo "  ${REGISTRY_BUILD_TAG}"

                    docker push \
                        ${REGISTRY_BUILD_TAG}


                    echo

                    echo "========================================"
                    echo "Docker push completed"
                    echo "========================================"
                """
            }
        }


        /*
         * ==========================================
         * 8. VERIFY REGISTRY
         * ==========================================
         */

        stage('Verify Registry') {
            steps {

                sh '''
                    set -e

                    echo "========================================"
                    echo "Verifying local Docker registry"
                    echo "========================================"

                    curl -fsS \
                        http://localhost:5000/v2/_catalog

                    echo

                    curl -fsS \
                        http://localhost:5000/v2/network-monitor/tags/list

                    echo

                    echo "Registry verification completed."
                '''
            }
        }


        /*
         * ==========================================
         * 9. DEPLOY FROM REGISTRY
         * ==========================================
         */

        stage('Deploy Container') {
            steps {

                sh """
                    set -e

                    echo "========================================"
                    echo "Deploying container from registry"
                    echo "========================================"

                    echo "Removing existing container..."

                    docker rm -f \
                        ${CONTAINER_NAME} 2>/dev/null || true


                    echo

                    echo "Pulling image from registry:"

                    echo "  ${REGISTRY_VERSION_TAG}"

                    docker pull \
                        ${REGISTRY_VERSION_TAG}


                    echo

                    echo "Starting container..."

                    docker run -d \
                        --name ${CONTAINER_NAME} \
                        -p ${HOST_PORT}:9000 \
                        ${REGISTRY_VERSION_TAG}


                    echo

                    echo "Container started:"

                    docker ps \
                        --filter "name=${CONTAINER_NAME}"
                """
            }
        }


        /*
         * ==========================================
         * 10. CONTAINER API TESTS
         * ==========================================
         */

        stage('Container API Tests') {
            steps {

                sh '''
                    set -e

                    echo "========================================"
                    echo "Waiting for Docker container"
                    echo "========================================"

                    for i in $(seq 1 20); do

                        if curl -fsS \
                            http://127.0.0.1:${HOST_PORT}/health \
                            > /dev/null; then

                            echo "Container is ready."

                            break
                        fi

                        sleep 1

                    done


                    echo
                    echo "========================================"
                    echo "Testing container /health"
                    echo "========================================"

                    curl -fsS \
                        http://127.0.0.1:${HOST_PORT}/health

                    echo


                    echo
                    echo "========================================"
                    echo "Testing container /system"
                    echo "========================================"

                    curl -fsS \
                        http://127.0.0.1:${HOST_PORT}/system

                    echo


                    echo
                    echo "========================================"
                    echo "Testing container /stats"
                    echo "========================================"

                    curl -fsS \
                        http://127.0.0.1:${HOST_PORT}/stats

                    echo


                    echo
                    echo "========================================"
                    echo "Container API tests passed"
                    echo "========================================"
                '''
            }
        }
    }


    /*
     * ==========================================
     * POST ACTIONS
     * ==========================================
     */

    post {

        success {

            echo """
========================================
CI/CD PIPELINE SUCCESS
========================================

Application version:
${APP_VERSION}

Git commit:
${GIT_COMMIT_SHORT}

Jenkins build:
${BUILD_NUMBER}

Registry images:

${REGISTRY_VERSION_TAG}
${REGISTRY_GIT_TAG}
${REGISTRY_BUILD_TAG}

========================================
"""
        }


        failure {

            echo """
========================================
CI/CD PIPELINE FAILED
========================================

Build:
${BUILD_NUMBER}

Check the Jenkins console output
for the failed stage.

========================================
"""
        }


        always {

            sh '''
                echo "========================================"
                echo "Cleaning up test container"
                echo "========================================"

                docker rm -f \
                    ${CONTAINER_NAME} 2>/dev/null || true
            '''
        }
    }
}
