## HW2

### 과제 설명 및 시연

본 과제는 double형 변수 정의하고 그 값에 따라 모터 돌리는 프로그램이다.

과제 1에서 사용한 코드를 약간 수정하여 사용하였다. 사용자로부터 입력 받는 volatile double 형의 변수를 만들고 이를 int16_t 형으로 변환하여 PWM과 Diration을 조정하였다.

(반)실시간으로 duty_rate 변수를 수정하기 위해 여러가지 방법을 시도해 보았다. 먼저 Live expressions에서 소수점을 보이게 한 뒤에 값을 입력 하려 하였으나 Live Expressions에서는 double을 입력할 때 상위 4개 바이트(1 word)와 하위 4개 바이트가 뒤바껴서 저장되어 0.1을 입력해도 이상한 숫자로 저장되는 문제가 있었다. 다음을 디버거툴의 메모리를 직접 접근하는 방식을 시도하였으나 실행도중에 값을 바꿀 수 없는데다가 한번에 32bit (4byte)의 공간만 수정할 수 있어 앞과 뒤가 깨지는 문제가 발생 할 수 있어서 포기하였다. 최종적으로는 일시정지 후 Dubugger Console에 "set var duty_rate = 0.8" 이런 형식으로 값을 수정하였다.

동작은 duty_rate의 예외값을 확인한 뒤 비율에 맞게 + 최대 70%로 맞춰서 Timer의 CCRn값을 조정한다. 방향은 부호를 확인하여 GPIO_WritePin SET, RESET을 이용한다.

시연 영상 : https://drive.google.com/file/d/14U6F71YU4UEt8vvot5nGVAgCeV2K_M2e/view?usp=sharing