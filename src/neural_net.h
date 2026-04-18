#ifndef NEURAL_NET_H
#define NEURAL_NET_H

#include <Eigen/Dense>
#include <string>
#include <vector>

class NeuralNet {
public:
    NeuralNet(int inputSize = 225, int hidden1 = 128, int hidden2 = 64);
    ~NeuralNet();

    float forward(const Eigen::VectorXf& input) const;
    void saveWeights(const std::string& filename) const;
    void loadWeights(const std::string& filename);
    void randomInit();
    void saveParams(const std::string& filename) const;
    void loadParams(const std::string& filename);

    int getInputSize() const { return inputSize; }

private:
    int inputSize, hidden1Size, hidden2Size;
    Eigen::MatrixXf w1, w2, w3;
    Eigen::VectorXf b1, b2, b3;
    void relu(Eigen::VectorXf& v) const;
    void sigmoid(Eigen::VectorXf& v) const;
};

#endif