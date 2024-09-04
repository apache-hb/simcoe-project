CREATE OR REPLACE FUNCTION IS_BLANK_STRING(text IN VARCHAR2) RETURN BOOLEAN IS
BEGIN
    IF REPLACE(REGEXP_REPLACE(text, '\s'), CHR(0)) IS NULL THEN
        RETURN TRUE;
    ELSE
        RETURN FALSE;
    END IF;
END;