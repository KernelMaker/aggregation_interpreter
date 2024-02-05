#include <iostream>
#include <fstream>
#include <random>

int main() {
  std::fstream fs;
  std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int64_t> distr_int64(-100000, 100000);
	std::uniform_int_distribution<uint64_t> distr_uint64(100000, 200000);
  std::uniform_real_distribution<double> distr_d1(-200000.0, 2000000.0);
  std::uniform_real_distribution<double> distr_d2(0.0, 5000000);
	std::uniform_int_distribution<int64_t> distr_int64_g(-2, 2);
	std::uniform_int_distribution<int64_t> distr_zero(1, 10);

  fs.open("data.txt", std::fstream::in | std::fstream::app);
  int64_t v1;
  double v2;
  uint64_t v3;
  double v4;
  int64_t v5;

	for (int i = 0; i < 1000000; i++) {
    v1 = distr_int64(gen);
    if (distr_zero(gen) == 6) {
      v2 = 0.0;
    } else {
      v2 = distr_d1(gen);
    }
    if (distr_zero(gen) == 6) {
      v3 = 0;
      if (v1 < 0) {
        v1 = 0 - v1;
      }
    } else {
      v3 = distr_uint64(gen);
    }
    if (distr_zero(gen) == 6) {
      v4 = 0.0;
    } else {
      v4 = distr_d2(gen);
    }
    v5 = distr_int64_g(gen);
    std::string str = std::to_string(v1) + "," + std::to_string(v2) + "," +
                      std::to_string(v3) + "," + std::to_string(v4) + "," +
                      std::to_string(v5);
    fs << str;
    fs << std::endl;
	}
  fs.close();

  return 0;
}
