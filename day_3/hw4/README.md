## HW4

### 과제 설명 및 시연

본 과제는 STM32CubeMonitor 사용해서 모터 Encoder값 받아서 비교해 보는 실습이다.

소스코드는 HW3을 그대로 사용하였고, CubeMonitor만 hw4에서 추가적으로 사용하였다. 따라서 HW4에는 README.md 파일만이 존재한다. CubeMonitor의 기초 사용법은 강의자료를 그대로 시행하였다. 

spd_cmd, enc_vel, enc_pos, enc_pos_total 4개의 변수를 화면에 띄운다. spd_cmd는 명령을 내리는 목표 각속도로써 10초에 한번씩 +3 <-> -3 을 사각파 형태로 진동한다. enc_vel은 모터의 실제 각속도로써 방향을 바꿀 때 약간의 딜레이가 존재하여 사다리꼴 파 형태가 된다.

 enc_pos는 모터가 내장된 엔코더를 이용해 응답하는 현재 각도로서 -4pi ~ 4pi의 범위를 띨 수 있다. 이때 원점에서 한 방향으로 2회전 이상 회전하면 오버플로우/언더플로우 되는 문제가 있다. 이를 해결하기 위해 직전값과 현재값 사이의 차의 크기를 계산하여 4pi 이상이면 오버/언더플로우 되었다고 판단하여 8pi를 더하거나 빼서 다시 이어 붙인다. 

시연 영상 : https://drive.google.com/file/d/19wDKI-AW-4fPPWCCggNkcGkR9wYwlq5S/view?usp=sharing