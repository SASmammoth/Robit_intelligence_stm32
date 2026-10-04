## HW3

### 과제 설명 및 시연

본 과제는  Robstride00 액추에이터를 CW/CCW 제어로 제어하는 프로그램이다.

강의자료애 'AI 활용가능'이라 명시적으로 나와있기 때문에 AI를 많이 사용하였다. 메뉴얼을 대충 훑어보니 6. MIT Communication Protocol 프로토콜이 눈에 띄기도 하고 간단해 보여서 MIT 프로토콜을 사용하려 하였다. 본격적인 사용 이전에 현재 모터가 어떤 프로토콜을 사용하고 있는지 확인하려고 코드를 돌려보니 4. Driver protocol and instructions 가 현재 프로토콜 이였다. 바꾸기도 뭐 해서 이 프로토콜을 사용하고 AI를 많이 이용하여 프로그램을 작성하였다. 중간에 모터가 안도는 문제가 있어서 모터ID를 다시 조회하여 ID 가 14가 아니라 1임을 알고 수정하였다.

24V에서 구동하였으며, 데이터시트의 최대 속도(33 rad/s)의 절반 미만인 3 rad/s로 설정하였다. 동작은 CCW/CW 방향으로 3rad/s씩 10초간격으로 회전한다.

시연 영상 : https://drive.google.com/file/d/1gCRGwYDuQ3G72-SmmsWjMRo6EfN5N7ui/view?usp=sharing