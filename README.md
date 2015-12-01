# KGC 2015 발표 자료

초간단 30분 안에 멀티플레이 게임서버 만들기

## 클라이언트

**아래와 같은 무료 오픈 소스를 사용하였습니다.**

- [CN Controls](https://www.assetstore.unity3d.com/en/?gclid=CNqTmfWduskCFQJwvAodBOIFBg#!/content/15233)
- ["Unity-chan!" Model](https://www.assetstore.unity3d.com/en/?gclid=CNqTmfWduskCFQJwvAodBOIFBg#!/content/18705)

## 서버

서버는 [iFunEngine](https://ifunfactory.com/engine/) 을 사용해서 작성했습니다.
정식 라이센스가 없어도 4주간 시험판을 사용해보실 수 있습니다.

### 빌드하기

 * 다음 linux 배포판을 지원합니다: Ubuntu 14.04, CentOS 7
 * iFun Engine을 설치해야 컴파일할 수 있습니다. 설치 방법은 [여기를 참고해주세요](https://www.ifunfactory.com/engine/documents/reference/ko/development-environment.html#installing-funapi)


### 포트 열기

예제 코드에서는 8022 번 포트를 TCP 연결을 위해 엽니다.

 * Ubuntu 14.04 의 경우 `sudo ufw allow 8022` 로 해당 포트를 열 수 있습니다.
 * CentOS 7 의 경우 `sudo firewall-cmd --zone=public --add-port=8022/tcp --permanent` 로 해당 포트를 열 수 있습니다.

*CentOS 7의 경우, `firewall-cmd --get-active-zones` 로 사용 중인 zone을 확인하고,
  `firewall-cmd --reload` 로 적용해야 동작합니다.*


## 문의/연락처

- [레퍼런스 매뉴얼](https://ifunfactory.com/engine/documents/reference/ko/)
- 라이센스 문의: sales@ifunfactory.com
