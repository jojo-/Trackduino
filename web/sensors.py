from flask import Flask, request, render_template
import serial
import datetime
from dateutil import tz

app = Flask(__name__)
app.debug = False

@app.route("/")
def gps_tracker():
  return render_template('gps-track.html')

@app.route("/gps", methods=['GET'])
def gps_db():

  date, latitude, longitude, altitude, satellites, speed, course = get_gps_records()

  if satellites > 0:
  
    # saving the data in a file gps.csv
    f = open("/home/pi/sensors/static/data/gps.csv", "a")
    data = str(latitude) + "," + str(longitude) + "," + str(date) + "," + str(satellites) + "," + str(speed) + "," + str(course) + "\n" 
    f.write(data)
    f.close()

  
  return("OK!")
  
def get_gps_records():

  import sqlite3

  # retrieving the data
  try:
    latitude = float(request.args.get('latitude'))
    longitude = float(request.args.get('longitude'))
    altitude = float(request.args.get('altitude'))
    date_str = str(request.args.get('date'))
    satellites = int(request.args.get('satellites'))
    speed = float(request.args.get('speed'))
    course = float(request.args.get('course'))
  except ValueError:
    latitude = 0
    longitude = 0
    altitude = 0
    date_str = "NA"
    date = datetime.datetime.now()
    satellites = 0
    speed = 0
    course = 0

  if satellites > 0:
    
    # converting date
    date = datetime.datetime.strptime(date_str, '%Y%m%d%H%M%S.%f')
    from_zone = tz.tzutc()
    to_zone = tz.gettz('Australia/ACT')
    date = date.replace(tzinfo=from_zone)
    date = date.astimezone(to_zone)
  
    # connect to database
    conn = sqlite3.connect('/var/www/sensors/gps.db')
    curs = conn.cursor()

    curs.execute("""INSERT INTO gps values((?), (?), (?), (?), (?), (?), (?))""",
               (date, latitude, longitude, altitude, satellites, speed, course))

    conn.commit()
    conn.close()
  
  return [date, latitude, longitude, altitude, satellites, speed, course]
  
if __name__ == "__main__":
  app.run(host='0.0.0.0', port=8080)

