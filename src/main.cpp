#include <iostream>

int v[5]{3, 48, 9, 3, 2};

void sort(int* arr, size_t size) {
  for (uint8_t i = 0; i < size; i++) {
    for (uint8_t j = i; j < size; j++) {
      if (arr[j] > arr[i]) {
        arr[i] = arr[j];
      }
    }
  }
}

int main(int argc, char** argv) {
  std::cout << "Before!\n" << std::endl;
  for (uint8_t i = 0; i < 5; i++) {
    std::cout << +v[i] << std::endl;
  }
  sort(v, 5);
  std::cout << "After!\n" << std::endl;
  for (uint8_t i = 0; i < 5; i++) {
    std::cout << +v[i] << std::endl;
  }


  return 0;
}
