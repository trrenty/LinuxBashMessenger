# LinuxBashMessenger
Linux C server enabling connected clients to send messages to one another whether they are online or not

The server initialises a data structure for each user in the conturi.txt 'database' and gives them an ID that corresponds to the line they belong to.

When an user connects, he has to login using the 'login nameInDatabase'. From this point onwards, he has access to a bunch of commands such as: 'onUsers'(gives you a list of online users), 'history otherUser'(shows all the message with the named user), 'idUser message' (sends message to the user with the given ID) and 'reply otherUser messageId message' (replies to the givenUser's messageId, number representing the index on the message in their history, with a message).

The Server and Cliend applications have the following Flow Schema:

![Flow Schema](https://github.com/trrenty/LinuxBashMessenger/blob/master/finaldiag.png)
