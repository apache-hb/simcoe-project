-- Get all log entries without any parameters
SELECT
	log_severity.name AS severity,
	datetime(round(log_entry.timestamp / 10000000), 'unixepoch') AS timestamp,
	log_message.path AS path,
	log_message.line AS line,
	log_message.function AS function,
	log_message.message AS message
FROM log_entry
INNER JOIN log_message ON log_entry.message_hash = log_message.hash
INNER JOIN log_severity ON log_message.severity = log_severity.severity
ORDER BY timestamp;

-- Get all log entries and print parameters

-- Get all log entries in most recent log session
SELECT
	log_severity.name AS severity,
	datetime(round(log_entry.timestamp / 10000000), 'unixepoch') AS timestamp,
	log_message.path AS path,
	log_message.line AS line,
	log_message.function AS function,
	log_message.message AS message,
	(
		SELECT group_concat(log_message_attribute.key || ' = ' || log_entry_attribute.value, ', ')
		FROM log_entry_attribute
		JOIN log_message_attribute ON log_message_attribute.message_hash = log_message.hash
		WHERE log_entry_attribute.entry_id = log_entry.id
		GROUP BY log_message_attribute.key
	) AS params
FROM log_entry
INNER JOIN log_message ON log_entry.message_hash = log_message.hash
INNER JOIN log_severity ON log_message.severity = log_severity.severity
ORDER BY timestamp;

-- Get all log entries in most recent log session
SELECT
	log_severity.name AS severity,
	datetime(round(log_entry.timestamp / 10000000), 'unixepoch') AS timestamp,
	log_message.path AS path,
	log_message.line AS line,
	log_message.function AS function,
	log_message.message AS message,
	(
		SELECT group_concat(log_message_attribute.key || ' = ' || log_entry_attribute.value, ', ')
		FROM log_entry_attribute
		JOIN log_message_attribute ON log_message_attribute.message_hash = log_message.hash
		WHERE log_entry_attribute.entry_id = log_entry.id
		GROUP BY log_message_attribute.key
	) AS params
FROM log_entry
INNER JOIN log_message ON log_entry.message_hash = log_message.hash
INNER JOIN log_severity ON log_message.severity = log_severity.severity
WHERE log_entry.timestamp > (SELECT max(start_time) FROM log_session)
ORDER BY timestamp;
