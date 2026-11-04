from . import Proc, Clean, REPO_TAR
from flask import Flask, jsonify, send_from_directory, abort

app = Flask(__name__)

#Finish HTTP Server to serve packages

if __name__ == '__main__':
    REPO_TAR.mkdir(parents=True, exist_ok=True)
    
    print(f"Serving tarballs directly from: {REPO_TAR}")
    app.run(host='127.0.0.1', port=8080, debug=True)