# SocketProgramming-ChatApplication

The server will be hosted on `localhost` and its port is `3205`. When the client connects successfully to the server, it will enter its private phone_number (username) initially. Then it is defined in theserver system. </br>

### The commands are described in below can be used in the system:

● `-gcreate phone_number+group_name`: Creates a new specified group. The
groups have been protected with non-encrypted passwords. The system will ask to
define a password. </br>
● `-join username/group_name`: Enter to the specified username or group name. 
If the group is private, the client must know the password for entering. </br>
● `-exit group_name`: Quit from the group that you are in. </br>
● `-send message_body`: Send a JSON-formatted message to the group that you are
in. </br>
● `-whoami` : Shows your own username (phone_number) information. </br>
● `-exit` : Exit the program. </br></br>

### Compile
`gcc threadSync.c –o outputfile –lpthread`
