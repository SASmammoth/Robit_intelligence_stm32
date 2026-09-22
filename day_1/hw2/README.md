## HW_2
---
예비단원 장경민

--- 

### 1단계 : adc_updated_flag 없이 while(1) 내부에서 adc1_buffer[0] 을 그냥 계속 읽으면 어떤 문제가 생길 수 있는가?

예상으로는 adc1_buffer[0]의 값을 읽을때 플래그가 없으면 타이밍에 따라 이 값이 이미 읽은 값인지 처음읽는 값은지 구분 할 수 없어 버퍼 등에 넣을 때 유효하지 않은 값이 나올 것 같다.


### 2단계

#### 2.1 PSD 정규화 (Min-Max Normalization)
최대값/최소값, 정규화 크기 등을 전역변수로 저장하여 값을 정규화 함. 정규화 스케일은 0~1000이다.

#### 2.2 adc1_buffer[0] 값에 이동평균 필터 적용
버퍼의 크기가 100인 이동평균 버퍼를 이용해서 이동평균을 구해 mov_mean_ 에 넣는다.

#### 2.3 원본값과 필터값 비교
영상 참조: 
https://drive.google.com/file/d/1kzjmrk5I4juCI9yivydvI6mQoxva5_Qd/view?usp=sharing


### 3단계: 위에서 만든 필터를 adc1_buffer[0]뿐 아니라 4채널 전부(buffer[0] ~ [3])에 똑같이 적용하려면
코드를 어떻게 바꿔야 하는가?

``` cpp

  while (1)
  {
    /* USER CODE END WHILE */
	  if(adc_update_flag)
	  {
		 adc_update_flag = 0;
		 adc1_value = adcl_buffer[0]; // 값 복사 원본 값
		
		/* 정규화 부분 생략 */
		
  		mov_mean_buffer[mov_mean_index] = adc1_value; // 버퍼에 원시 값 추가
  		mov_mean_index++; // 인덱스에 1 추가
  		if(mov_mean_index >= MOV_MEAN_SIZE) mov_mean_index = 0;

  		uint32_t temp_sum = 0;
  		for(int i = 0; i < MOV_MEAN_SIZE; i++)
  		{
  			temp_sum += mov_mean_buffer[i]; // 값 추가
  		}

		  mov_mean_ = temp_sum / MOV_MEAN_SIZE; // 최종 이동평균 값 구함
	  }
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */

```
이 부분을

```cpp

while (1)
{
  /* USER CODE END WHILE */
  if(adc_update_flag)
  {
	 adc_update_flag = 0;
	 for(int i = 0; i < 4; i++)
	 {
	 	adc_value[i] = adcl_buffer[i]; // 값 복사 원본 값

		mov_mean_buffer[i][mov_mean_index[i]] = adc_value[i]; // 버퍼에 원시 값 추가
		mov_mean_index[i]++; // 인덱스에 1 추가
		if(mov_mean_index[i] >= MOV_MEAN_SIZE) mov_mean_index[i] = 0;

		uint32_t temp_sum = 0;
		for(int j = 0; j < MOV_MEAN_SIZE; j++)
		{
			temp_sum += mov_mean_buffer[i][j]; // 값 추가
		}

	  mov_mean_[i] = temp_sum / MOV_MEAN_SIZE; // 최종 이동평균 값 구함
	  }
  /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

```

이러한 형식으로 수정한다. 힌트에서는 필터 로직을 함수로 뽑아내고, 채널별로 별도의 이전값 저장 공간이 필요하다고 하였다. 하지만 내 코드에서는 이미 필수적인 변수는 전역변수에 있고 필터 로직을 메인에다가 넣었기 때문에 이를 반복문으로 감싸는 형식으로 수정하였다. 만약 힌트대로 하고 싶으면 저 부분을 함수로 따로 빼기만 하면 된다.

### 4단계: flag 방식 대신, buffer 인덱스가 바뀌는 것을 직접 감지하는 다른 방법을 설계하기.

DMA방식중 버퍼 2개를 이용해서 두 버퍼를 번갈아 가면서 체우는 방식이 있다.(HAL_DMAEx_MultiBufferStart) 이 방식에서는 하나의 버퍼가 전부 체워지면 CT비트가 자동으로 반전되는데, 이 값을 읽어 반전될 때 버퍼에서 값을 읽어오면 된다.