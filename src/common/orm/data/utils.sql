CREATE OR REPLACE FUNCTION IS_BLANK_STRING(text CHARACTER VARYING) RETURN BOOLEAN IS
BEGIN
    RETURN REPLACE(REGEXP_REPLACE(text, '\s'), CHR(0)) = '';
END;
