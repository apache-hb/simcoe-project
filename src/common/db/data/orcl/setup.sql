CREATE OR REPLACE FUNCTION IS_BLANK_STRING(text IN VARCHAR2) RETURN BOOLEAN IS
BEGIN
    RETURN REGEXP_LIKE(text, '\s');
END;
