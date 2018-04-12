# ATM-simulator
Simulator for ATM using both tcp and upd sockets

ATM simulator via TCP and UDP sokets
TCP channel user for atm services:
  -login
  -logout
  -balance
  -getmoney
  -putmoney
  -quit
UPD channel used for unlock services(if the user enters 3 times in a row a bad password the account
is blocked)

Usage:
  ./server <server_port> <users_data_file>
  users_data_file entry format:
    <last name> <first name> <card number> <pin> <secret password> <balance>
  ./client <server_IP> <server_port>

   
  
