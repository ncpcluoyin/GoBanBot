#include "neural_net.h"
#include <fstream>
#include <random>
#include <iostream>

using namespace Eigen;

NeuralNet::NeuralNet(int in, int h1, int h2)
    : inputSize(in), hidden1Size(h1), hidden2Size(h2) {
    randomInit();
}

NeuralNet::~NeuralNet() {}

void NeuralNet::relu(VectorXf& v) const { v = v.array().max(0.0f); }
void NeuralNet::sigmoid(VectorXf& v) const { v = 1.0f / (1.0f + (-v.array()).exp()); }

float NeuralNet::forward(const VectorXf& input) const {
    VectorXf h1 = w1 * input + b1;
    relu(h1);
    VectorXf h2 = w2 * h1 + b2;
    relu(h2);
    VectorXf out = w3 * h2 + b3;
    sigmoid(out);
    return out(0);
}

void NeuralNet::randomInit() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> dist(0, 0.1);
    auto rand = [&]() { return dist(gen); };
    w1 = MatrixXf::NullaryExpr(hidden1Size, inputSize, rand);
    b1 = VectorXf::NullaryExpr(hidden1Size, rand);
    w2 = MatrixXf::NullaryExpr(hidden2Size, hidden1Size, rand);
    b2 = VectorXf::NullaryExpr(hidden2Size, rand);
    w3 = MatrixXf::NullaryExpr(1, hidden2Size, rand);
    b3 = VectorXf::NullaryExpr(1, rand);
}

void NeuralNet::saveWeights(const std::string& filename) const {
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) { std::cerr << "Cannot save weights to " << filename << std::endl; return; }
    auto write = [&](const auto& m) { ofs.write(reinterpret_cast<const char*>(m.data()), m.size() * sizeof(float)); };
    write(w1); write(b1); write(w2); write(b2); write(w3); write(b3);
    std::cout << "Weights saved to " << filename << std::endl;
}

void NeuralNet::loadWeights(const std::string& filename) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) { std::cerr << "Cannot load weights from " << filename << std::endl; return; }
    auto read = [&](auto& m) { ifs.read(reinterpret_cast<char*>(m.data()), m.size() * sizeof(float)); };
    read(w1); read(b1); read(w2); read(b2); read(w3); read(b3);
    std::cout << "Weights loaded from " << filename << std::endl;
}

void NeuralNet::saveParams(const std::string& filename) const {
    std::ofstream ofs(filename);
    if (ofs) {
        ofs << inputSize << " " << hidden1Size << " " << hidden2Size << std::endl;
    }
}

void NeuralNet::loadParams(const std::string& filename) {
    std::ifstream ifs(filename);
    if (ifs) {
        ifs >> inputSize >> hidden1Size >> hidden2Size;
        randomInit(); // 重新分配大小
    }
}