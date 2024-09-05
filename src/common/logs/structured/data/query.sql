SELECT
	log_severity.name AS name,
	datetime(round(log_entry.timestamp / 10000000), 'unixepoch') AS timestamp,
	log_message.file AS file,
	log_message.line AS line,
	log_message.function AS function,
	log_message.message AS message
FROM log_entry
INNER JOIN log_message ON log_entry.id = log_message.id
INNER JOIN log_severity ON log_message.level = log_severity.level
ORDER BY timestamp;
