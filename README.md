Some of the source code is written in C++, and so you will need the g++ cross-compiler installed on your Debian host machine to build the app.
Run these commands to do so:
`sudo apt update`
`sudo apt install g++-arm-linux-gnueabihf`

To run the nodejs server:
Run make in the `NodeJs-Code` folder, 
run the command `node server.js` in the `/mnt/remote/node/nodejs-code-copy` folder on the target, 
navigate to `192.168.7.2:8088` on your web browser
