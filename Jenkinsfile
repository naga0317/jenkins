pipeline {
    agent any

    options {
        timestamps()
        disableConcurrentBuilds()

        buildDiscarder(
            logRotator(
                numToKeepStr: '10'
            )
        )
    }

    environment {
        IMAGE_NAME = "network-monitor"
        REGISTRY = "localhost:5000"
        REGISTRY_IMAGE = "${REGISTRY}/${IMAGE_NAME}"

        RETAIN_BUILDS = "2"

        K8S_DEPLOYMENT = "network-monitor"
        K8S_CONTAINER = "network-monitor"

        HOST_PORT = "19000"
    }

    stages {

        stage('Checkout') {
            steps {
                checkout scm

                sh '''
                    set -e

                    echo "======================================"
                    echo "Git Information"
                    echo "======================================"

                    git rev-parse --short HEAD
                    git log -1 --oneline
                '''
            }
        }


        stage('Build C++ Application') {
            steps {
                sh '''
                    set -e

                    echo "======================================"
                    echo "Building C++ Application"
                    echo "======================================"

                    rm -rf build

                    cmake -S . -B build
                    cmake --build build -j$(nproc)

                    echo
                    echo "Build completed successfully."
                '''
            }
        }


        stage('Unit Tests') {
            steps {
                sh '''
                    set -e

                    echo "======================================"
                    echo "Running Unit Tests"
                    echo "======================================"

                    cd build
                    ctest --output-on-failure

                    echo
                    echo "Unit tests passed."
                '''
            }
        }


        stage('Native API Tests') {
            steps {
                sh '''
                    set -e

                    echo "======================================"
                    echo "Native API Tests"
                    echo "======================================"

                    ./build/network-monitor > native-server.log 2>&1 &

                    APP_PID=$!

                    cleanup() {
                        kill "$APP_PID" 2>/dev/null || true
                    }

                    trap cleanup EXIT

                    echo "Waiting for application..."

                    READY=0

                    for i in $(seq 1 20); do
                        if curl -fsS \
                            http://127.0.0.1:9000/health \
                            > /dev/null; then

                            READY=1
                            echo "Application is ready."
                            break
                        fi

                        sleep 1
                    done

                    if [ "$READY" -ne 1 ]; then
                        echo "Application failed to start."
                        cat native-server.log
                        exit 1
                    fi

                    echo
                    echo "Testing /health"

                    curl -fsS \
                        http://127.0.0.1:9000/health

                    echo

                    echo
                    echo "Testing /system"

                    curl -fsS \
                        http://127.0.0.1:9000/system

                    echo

                    echo
                    echo "Testing /stats"

                    curl -fsS \
                        http://127.0.0.1:9000/stats

                    echo

                    echo
                    echo "Native API tests passed."
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

                        echo "======================================"
                        echo "Building Docker Image"
                        echo "======================================"

                        echo "Image:"
                        echo "${BUILD_IMAGE_TAG}"

                        docker build \
                            -t ${BUILD_IMAGE_TAG} \
                            .

                        echo
                        echo "Docker image built successfully."

                        docker images ${BUILD_IMAGE_TAG}
                    """
                }
            }
        }


        stage('Docker Image Smoke Test') {
            steps {
                sh """
                    set -e

                    echo "======================================"
                    echo "Docker Image Smoke Test"
                    echo "======================================"

                    CONTAINER_NAME="network-monitor-smoke-${BUILD_NUMBER}"

                    cleanup() {
                        docker rm -f "\$CONTAINER_NAME" \
                            2>/dev/null || true
                    }

                    trap cleanup EXIT

                    echo "Starting container..."

                    docker run -d \
                        --name "\$CONTAINER_NAME" \
                        -p ${HOST_PORT}:9000 \
                        ${BUILD_IMAGE_TAG}

                    echo
                    echo "Waiting for container..."

                    READY=0

                    for i in \$(seq 1 20); do
                        if curl -fsS \
                            http://127.0.0.1:${HOST_PORT}/health \
                            > /dev/null; then

                            READY=1
                            echo "Container is ready."
                            break
                        fi

                        sleep 1
                    done

                    if [ "\$READY" -ne 1 ]; then
                        echo "Container failed to start."

                        docker logs "\$CONTAINER_NAME"

                        exit 1
                    fi

                    echo
                    echo "Testing container /health"

                    curl -fsS \
                        http://127.0.0.1:${HOST_PORT}/health

                    echo

                    echo
                    echo "Testing container /system"

                    curl -fsS \
                        http://127.0.0.1:${HOST_PORT}/system

                    echo

                    echo
                    echo "Testing container /stats"

                    curl -fsS \
                        http://127.0.0.1:${HOST_PORT}/stats

                    echo

                    echo
                    echo "Docker image smoke test passed."
                """
            }
        }


        stage('Push Docker Image') {
            steps {
                sh """
                    set -e

                    echo "======================================"
                    echo "Pushing Docker Image"
                    echo "======================================"

                    echo "Pushing:"
                    echo "${BUILD_IMAGE_TAG}"

                    docker push ${BUILD_IMAGE_TAG}

                    echo
                    echo "Docker push completed."
                """
            }
        }


        stage('Registry Retention - Keep Last 2') {
            steps {
                sh '''
                    set -e

                    echo "======================================"
                    echo "Registry Retention Policy"
                    echo "======================================"

                    echo "Keeping latest 2 build images."

                    TAGS=$(curl -fsS \
                        http://localhost:5000/v2/network-monitor/tags/list)

                    echo
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

delete = build_tags[2:]

for number, tag in delete:
    print(tag)
')

                    if [ -z "$OLD_TAGS" ]; then
                        echo
                        echo "No old build images to delete."
                    else
                        echo
                        echo "Old build images to delete:"
                        echo "$OLD_TAGS"

                        for TAG in $OLD_TAGS; do
                            echo
                            echo "Processing:"
                            echo "$TAG"

                            DIGEST=$(curl -fsSI \
                                -H "Accept: application/vnd.docker.distribution.manifest.v2+json" \
                                "http://localhost:5000/v2/network-monitor/manifests/$TAG" \
                                2>/dev/null \
                                | awk -F': ' '
                                    tolower($1)=="docker-content-digest" {
                                        print $2
                                        exit
                                    }
                                ' \
                                | tr -d '\\r')

                            if [ -z "$DIGEST" ]; then
                                echo "Tag $TAG is already absent from the registry; skipping deletion."
                                continue
                            fi

                            echo "Digest:"
                            echo "$DIGEST"

                            echo "Deleting manifest..."

                            if ! curl -fsS \
                                -X DELETE \
                                "http://localhost:5000/v2/network-monitor/manifests/$DIGEST"; then
                                echo "Failed to delete $TAG ($DIGEST); continuing with remaining tags."
                                continue
                            fi

                            echo
                            echo "Deleted:"
                            echo "$TAG"
                        done
                    fi

                    echo
                    echo "======================================"
                    echo "Registry Tags After Cleanup"
                    echo "======================================"

                    curl -fsS \
                        http://localhost:5000/v2/network-monitor/tags/list

                    echo
                '''
            }
        }


        stage('Deploy to Kubernetes') {
            steps {
                sh """
                    set -e

                    echo "======================================"
                    echo "Deploying to Kubernetes"
                    echo "======================================"

                    if ! kubectl config current-context >/dev/null 2>&1; then
                        echo "Kubernetes context is not configured in this environment; skipping deployment."
                        echo "Target image: ${BUILD_IMAGE_TAG}"
                        exit 0
                    fi

                    echo
                    echo "Kubernetes context:"
                    kubectl config current-context

                    echo
                    echo "Target image:"
                    echo "${BUILD_IMAGE_TAG}"

                    echo
                    echo "Patching deployment manifest with build image..."
                    BUILD_IMAGE_TAG="${BUILD_IMAGE_TAG}" python3 - <<'PY'
import os

path = 'k8s/deployment.yaml'
image = os.environ['BUILD_IMAGE_TAG']

with open(path, 'r', encoding='utf-8') as f:
    lines = f.readlines()

updated = False
for i, line in enumerate(lines):
    if line.strip().startswith('image:'):
        lines[i] = '          image: ' + image + '\n'
        updated = True
        break

if not updated:
    raise SystemExit('Expected one image field in k8s/deployment.yaml')

with open(path, 'w', encoding='utf-8') as f:
    f.writelines(lines)
PY

                    echo
                    echo "Applying Kubernetes manifests..."
                    kubectl apply -f k8s/configmap.yaml
                    kubectl apply -f k8s/deployment.yaml
                    kubectl apply -f k8s/service.yaml

                    echo
                    echo "Current Deployment image:"
                    kubectl get deployment \
                        ${K8S_DEPLOYMENT} \
                        -o jsonpath='{.spec.template.spec.containers[0].image}'

                    echo
                """
            }
        }


        stage('Kubernetes Rollout Status') {
            steps {
                sh '''
                    set -e

                    echo "======================================"
                    echo "Kubernetes Rolling Update"
                    echo "======================================"

                    if ! kubectl config current-context >/dev/null 2>&1; then
                        echo "Kubernetes context is not configured in this environment; skipping rollout status."
                        exit 0
                    fi

                    kubectl rollout status \
                        deployment/${K8S_DEPLOYMENT} \
                        --timeout=180s

                    echo
                    echo "Deployment status:"
                    kubectl get deployment ${K8S_DEPLOYMENT}

                    echo
                    echo "Pods:"
                    kubectl get pods \
                        -l app=network-monitor \
                        -o wide

                    echo
                    echo "ReplicaSets:"
                    kubectl get replicasets \
                        -l app=network-monitor
                '''
            }
        }


        stage('Kubernetes API Tests') {
            steps {
                sh '''
                    set -e

                    echo "======================================"
                    echo "Kubernetes API Tests"
                    echo "======================================"

                    if ! kubectl config current-context >/dev/null 2>&1; then
                        echo "Kubernetes context is not configured in this environment; skipping API tests."
                        exit 0
                    fi

                    echo
                    echo "Testing Kubernetes Service..."
                    kubectl get service ${K8S_DEPLOYMENT}

                    echo
                    echo "Testing /health"

                    TEST_POD="network-monitor-api-test-${BUILD_NUMBER}-health"

                    kubectl run "$TEST_POD" \
                        --restart=Never \
                        --image=curlimages/curl \
                        --command -- \
                        curl -fsS \
                        http://network-monitor:9000/health

                    kubectl wait \
                        --for=jsonpath='{.status.phase}'=Succeeded \
                        pod/"$TEST_POD" \
                        --timeout=60s

                    kubectl delete pod "$TEST_POD" \
                        --ignore-not-found=true \
                        --wait=true


                    echo
                    echo "Testing /system"

                    TEST_POD="network-monitor-api-test-${BUILD_NUMBER}-system"

                    kubectl run "$TEST_POD" \
                        --restart=Never \
                        --image=curlimages/curl \
                        --command -- \
                        curl -fsS \
                        http://network-monitor:9000/system

                    kubectl wait \
                        --for=jsonpath='{.status.phase}'=Succeeded \
                        pod/"$TEST_POD" \
                        --timeout=60s

                    kubectl delete pod "$TEST_POD" \
                        --ignore-not-found=true \
                        --wait=true


                    echo
                    echo "Testing /stats"

                    TEST_POD="network-monitor-api-test-${BUILD_NUMBER}-stats"

                    kubectl run "$TEST_POD" \
                        --restart=Never \
                        --image=curlimages/curl \
                        --command -- \
                        curl -fsS \
                        http://network-monitor:9000/stats

                    kubectl wait \
                        --for=jsonpath='{.status.phase}'=Succeeded \
                        pod/"$TEST_POD" \
                        --timeout=60s

                    kubectl delete pod "$TEST_POD" \
                        --ignore-not-found=true \
                        --wait=true


                    echo
                    echo "======================================"
                    echo "All Kubernetes API tests passed"
                    echo "======================================"
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

Jenkins build:
${BUILD_NUMBER}

Docker image:
${BUILD_IMAGE_TAG}

Registry:
${REGISTRY}

Registry policy:
Keep latest 2 build-* images

Kubernetes:
Deployment = ${K8S_DEPLOYMENT}

========================================
"""
        }


        failure {
            echo """
========================================
CI/CD PIPELINE FAILED
========================================

Jenkins build:
${BUILD_NUMBER}

Docker image:
${BUILD_IMAGE_TAG}

========================================
"""
        }


        always {
            sh '''
                echo "Cleaning temporary Docker containers..."

                docker rm -f \
                    network-monitor-smoke-${BUILD_NUMBER} \
                    2>/dev/null || true
            '''
        }
    }
}
