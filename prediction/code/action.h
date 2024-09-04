//에어백 전개 결정에 필요한 함수 모음.

//입력 값이 0 이상이면 그대로 반환, 음수면 0을 반환
double relu(double a) {
  if (a < 0) {
    return 0;
  }
  else {
    return a;
  }
}

//에어백 전개 여부 결정 함수
bool isAirbag(double input[]) {
  //double input[] = {263,30.71,-5,0.539}; - 입력 데이터는 거리,속도,가속도, 마찰계수로 구성

  //입력값을 표준화 (Z = (입력 - 평균) / 표준편차)
  for (int i = 0; i < Nnodelayer[0]; i++) {
    input[i] = (input[i] - mean[i]) / standard[i]; 
  }

  double* node[4] = { node_0, node_1, node_2, node_3 }; //node 배열 선언

  //node 배열의 0행을 표준화된 입력값으로 초기화
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < Nnodelayer[i]; j++) {
      if (i == 0) {
        node[i][j] = input[j];
      }
    }
  } 

  double weight[3][20][10]; //weight 3차원 배열 선언

  //weight 배열에 data.h의 가중치 데이터 대입
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 20; j++) {
      weight[0][i][j] = weight_0[i][j];
    }
  }
  for (int i = 0; i < 20; i++) {
    for (int j = 0; j < 10; j++) {
      weight[1][i][j] = weight_1[i][j];
    }
  }
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 2; j++) {
      weight[2][i][j] = weight_2[i][j];
    }
  }; 

  double* bias[3] = { bias_0, bias_1, bias_2 }; // bias 2차원 배열 선언 및 data.h의 bias 데이터 대입

  //node 배열에 가중치를 각각 곱해 합한 후 bias 값을 더해 0 이상이면 그대로, 0 이하면 0으로 node 배열에 저장
  for (int i = 0; i < 4; i++) {
    if (i == 0) {
    }
    else {
      for (int k = 0; k < Nnodelayer[i]; k++) {
        double sum = 0;
        for (int j = 0; j < Nnodelayer[i - 1]; j++) {
          sum = sum + node[i - 1][j] * weight[i - 1][j][k];
        }
        sum = sum + bias[i - 1][k];
        sum = relu(sum);
        node[i][k] = sum;
      }
    }
  }

  bool operation = 0; //에어백 전개 여부
  double prob = 0; //충돌 확률

  //node에서 확률에 따라 에어백 전개 여부를 결정
  for (int i = 0; i < Nnodelayer[3]; i++) {
    if (node[3][i] > prob) {
      operation = i;
      prob = node[3][i];
    }
  }

  return operation; //에어백 전개 여부 반환
}