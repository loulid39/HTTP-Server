#!/bin/bash


echo "acces au : loulic.txt"

curl http://127.0.0.1:2143/loulic.txt
echo -e "\nacces au ru.html\n"
curl http://127.0.0.1:2143/ru.html
echo -e "\nacces au index.html\n"
curl http://127.0.0.1:2143/index.html
echo -e "\nacces au bin"
curl http://127.0.0.1:2143/bin
echo -e "\nacces au loop.sh"
curl http://127.0.0.1:2143/loop.sh
echo -e "\ntest deni de service\n"
curl http://127.0.0.1:2143/index.html
curl http://127.0.0.1:2143/index.html
curl http://127.0.0.1:2143/index.html

exit $?





		


