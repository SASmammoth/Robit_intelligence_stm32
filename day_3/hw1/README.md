## HW1

### 과제 설명 및 시연

본 과제는 주기 20KHz PWM을 생성하고 duty를 0%, 30%, 50%, 70% 

실제 PWM이 동작하는 모습을 보이기 위해 DC모터에 연결하여 동작을 확인하였다. 강의자료에서는 모터를 동작하기 위한 PWM출력이 PC8, 모터 방향 출력이 PA8인거 처럼 적혀있고 예제로 Timer8을 사용한다. 하지만 실제 STM mainboard에서는 Moter_PWM이 PA8, Moter_Dir이 PC8 로 구성되어 있어 Timer8 대신 Timer1이용하여 코드를 작성하였다.

동작은 간단하게 2000ms마다 duty를 0% -> 30% -> 50%-> 70%로 바꾼다. 20KHz 주기를 구성하기위해 Prescaler 18-1에 Counter Period 500-1로 하였다. ( 180000000 / 18 / 500 = 20000 )

시연 영상 : https://drive.google.com/file/d/1lzh34l4dpfRdBDz0HEMwsHGzdDR1AmJs/view?usp=sharing