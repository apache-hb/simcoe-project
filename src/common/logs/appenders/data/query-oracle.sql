CREATE OR REPLACE FUNCTION EPOCH_TO_TIMESTAMP(epoch IN NUMBER) RETURN TIMESTAMP
IS
BEGIN
    RETURN to_timestamp('19700101', 'YYYYMMDD') + ( 1 / 24 / 60 / 60 / 1000) * (epoch);
END EPOCH_TO_TIMESTAMP;
/

SELECT
	log_severity.name AS severity,
	EPOCH_TO_TIMESTAMP(log_entry.timestamp) AS timestamp,
	log_message.path AS path,
	log_message.line AS line,
	log_message.function AS function,
	log_message.message AS message,
	(
		SELECT listagg(log_message_attribute.key || ' = ' || log_entry_attribute.value, ', ')
            WITHIN GROUP (ORDER BY log_message_attribute.key)
		FROM log_entry_attribute
        INNER JOIN log_message_attribute ON log_message_attribute.message_hash = log_message.hash
		WHERE log_entry_attribute.entry_id = log_entry.id
	) AS params
FROM log_entry
INNER JOIN log_message ON log_entry.message_hash = log_message.hash
INNER JOIN log_severity ON log_message.severity = log_severity.severity
ORDER BY timestamp;
