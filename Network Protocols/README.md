# GBN
## To run
python3 sender.py <file_to_be_sent>
python3 receiver.py <file_to_be_written>

# SW
For SW make the window size in GBN in sender.py to 1 and run exactly the same way as GBN.


# SR
## To run
python2.7 ClientApp.py -f "loco.jpg" -a "127.0.0.1" -b 8081 -x "127.0.0.1" -y 8080 -m 5 -w 7 -s 25000 -n "ALL" -t 0.2 &

python2.7 ServerApp.py -a "127.0.0.1" -b 8081 -x "127.0.0.1" -y 8080 -m 5 -w 7 -f "loco.jpg" -t 0.2 &


