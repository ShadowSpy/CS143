LOAD xsmall FROM 'xsmall.del' WITH INDEX
SELECT COUNT(*) FROM xsmall
SELECT * FROM xsmall WHERE key < 2500

LOAD small FROM 'small.del' WITH INDEX
SELECT COUNT(*) FROM small
SELECT * FROM small WHERE key > 100 AND key < 500

LOAD medium FROM 'medium.del' WITH INDEX
SELECT COUNT(*) FROM medium
SELECT * FROM medium WHERE key = 489

LOAD large FROM 'large.del' WITH INDEX
SELECT COUNT(*) FROM large
SELECT * FROM large WHERE key > 4500
SELECT * FROM large WHERE key > 4500 AND key > 0

LOAD xlarge FROM 'xlarge.del' WITH INDEX
SELECT COUNT(*) FROM xlarge
SELECT * FROM xlarge WHERE key = 4240
SELECT * FROM xlarge WHERE key > 400 AND key < 500 AND key > 100 AND key < 4000000

