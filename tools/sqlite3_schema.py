#
# Initialize a SQLite database with the `World` table with 10K rows.
#
# python sqlite3_schema.py | sqlite3 database.db
#
import random

k = 10000
items = random.sample(range(k), k=k)

print("BEGIN TRANSACTION;")

print("""CREATE TABLE world (
    id INTEGER PRIMARY KEY ASC,
    randomNumber INTEGER NOT NULL DEFAULT 0
);""")

for i, j in enumerate(items):
    print(f"INSERT INTO world VALUES ({i + 1}, {j + 1});")

print("COMMIT;")
