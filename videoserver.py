from flask import Flask, Response
import cv2

app = Flask(__name__)

def generate_video():
    cap = cv2.VideoCapture(0)

    # Настройки камеры для снижения нагрузки
    cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 360)
    cap.set(cv2.CAP_PROP_FPS, 10)

    if not cap.isOpened():
        print("❌ Камера не открылась")
        return

    while True:
        ret, frame = cap.read()
        if not ret:
            continue

        # MJPEG-сжатие
        _, buffer = cv2.imencode('.jpg', frame)
        frame_bytes = buffer.tobytes()

        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')

@app.route('/video_feed')
def video_feed():
    return Response(generate_video(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/')
def index():
    return "📷 USB Video Stream is running on /video_feed"

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8000, debug=False, threaded=True)
