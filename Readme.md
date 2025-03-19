[Unit]
Description=Staring gasDataServer Service by Hans
After=multi-user.target

[Service]
WorkingDirectory = /home/hanskim/gasDataServer
ExecStart=/home/hanskim/gasDataServer/server1
Restart=always
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target