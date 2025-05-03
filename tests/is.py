import pymysql

try:
  import mdb
  mdb_module = "enabled"
except ImportError:
  mdb_module = "disabled"

iterations=50

conn= pymysql.connect(unix_socket="/tmp/mysql.sock")

cursor=conn.cursor()
cursor.execute("SELECT count(*) FROM information_schema.columns")
row= cursor.fetchone()

print(f"running test ({iterations} iterations): select {row[0]} records from information_schema.columns")
print(f"mdb module is {mdb_module}.")


for i in range(0,iterations):
    cursor.execute("SELECT * FROM information_schema.columns")
    rows= cursor.fetchall()

cursor.close()
