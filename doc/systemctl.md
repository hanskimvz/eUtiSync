

# systemd 서비스 등록 방법 (CentOS 및 Ubuntu)
```
[Unit]
Description=Staring gasDataServer Service by Hans
After=multi-user.target

[Service]
WorkingDirectory=/home/hanskim/eUtiSync
ExecStart=/home/hanskim/eUtiSync/eutisync
Restart=always
RestartSec=10
StandardOutput=syslog
StandardError=syslog
SyslogIdentifier=eutisync
User=hanskim
Group=hanskim
Environment=NODE_ENV=production

[Install]
WantedBy=multi-user.target
```


## 서비스 파일 설치 방법 (공통)

1. 서비스 파일을 시스템 디렉토리에 복사:
```
sudo cp eUtiSync.service /etc/systemd/system/eutisync.service
```

2. 서비스 파일 권한 설정:
```
sudo chmod 644 /etc/systemd/system/eutisync.service
```

3. systemd 데몬 리로드:
```
sudo systemctl daemon-reload
```

4. 서비스 활성화 (부팅 시 자동 시작):
```
sudo systemctl enable eutisync.service
```

5. 서비스 시작:
```
sudo systemctl start eutisync.service
```

6. 서비스 상태 확인:
```
sudo systemctl status eutisync.service
```

## CentOS와 Ubuntu의 차이점

### 로그 확인 방법
- CentOS 7: `journalctl -u eutisync.service` 또는 `/var/log/messages`
- Ubuntu: `journalctl -u eutisync.service` 또는 `/var/log/syslog`

### SELinux 관련 (CentOS)
CentOS에서는 SELinux가 기본적으로 활성화되어 있어 추가 설정이 필요할 수 있습니다:
```
sudo chcon -t systemd_unit_file_t /etc/systemd/system/eutisync.service
```

### 방화벽 설정
- CentOS: `firewall-cmd`
- Ubuntu: `ufw`

## 추가 팁

1. 서비스 파일의 경로는 절대 경로로 지정하는 것이 좋습니다.
2. User와 Group 설정을 통해 보안을 강화할 수 있습니다.
3. 로그 설정을 통해 문제 발생 시 디버깅이 용이해집니다.
4. RestartSec 설정으로 서비스 재시작 간격을 조절할 수 있습니다.