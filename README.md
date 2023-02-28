Run ./installDebianPackages.sh to install dependencies. Do this before running `make` for the first time.

To make the entire project in one command, run `make`.
To make only the C++ code, run `make cpp_app`.
To make only the NodeJS code, run `make node`.

To run the app:
- Run `./beatbox` in the `/mnt/remote/myApps` directory on the target
- Run `node server.js` in the `/mnt/remote/myApps/beatbox-server-copy` folder on the target
- Connect to `192.168.7.2:8088` via your host machine's web browser
