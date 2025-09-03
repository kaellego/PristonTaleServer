#pragma once
#include <random>

class Dice {
public:
    // Gera um número inteiro entre min e max (inclusivo)
    static int RandomI(int min, int max) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(min, max);
        return distrib(gen);
    }

    // Gera um número de ponto flutuante entre min e max
    static float RandomF(float min, float max) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> distrib(min, max);
        return distrib(gen);
    }

    // Template genérico para outros tipos (se necessário)
    template<typename T>
    static T RandomValue(T min, T max) {
        // Implementação simplificada, pode precisar de especialização
        if constexpr (std::is_integral_v<T>) {
            return static_cast<T>(RandomI(static_cast<int>(min), static_cast<int>(max)));
        }
        else if constexpr (std::is_floating_point_v<T>) {
            return static_cast<T>(RandomF(static_cast<float>(min), static_cast<float>(max)));
        }
        return min;
    }
};