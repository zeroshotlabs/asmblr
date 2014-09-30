<?php
/**
 * @file SQLSrv.inc SQL Server connectivity.
 * @author Stackware, LLC
 * @version 4.0
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * This extension is comprised of two classes:
 *  - The SQLSrv class which wraps PHP's sqlsrv_* functions and implements both
 *    Debuggable and CRUDC, and includes debugging, error handling, data mapping
 *    and binding, and SQL generation.
 *
 *  - The SQLSrvSet class which extends TemplateSet for custom SQL statements.
 *
 * The SQLSrv extension provides two ways to query the database:
 *  - SQLSrv/CRUDC: Simple CREATE/READ/UPDATE/DELETE/COUNT queries with automatic
 *    SQL generation and parameter binding.  Queries are executed using a fixed set of
 *    methods and customization is limited.  Column nor table names are escaped.
 *
 *  - SQLSrvSet/TemplateSet: Specialized Templates for dynamically generating
 *    custom SQL with complex SQL generation.  Queries are rendered and executed
 *    by Name.
 *
 * Both classes use sqlsrv_query() for executing statements and assist in
 * correctly binding parameters.  However, not all functionality of the sqlsrv
 * PHP extension is supported.
 *
 * @todo Thorough testing of all functionality and debugging.
 */
class SQLSrv implements Debuggable,CRUDC
{
    use Debugged;

    /**
     * Cache of table column info (name, type and length).
     */
    protected $ColumnInfoCache = array();

    /**
     * SQL Server database resource.
     */
    public $DB;

    /**
     * Create a SQLSrv object and connect to a database.
     *
     * @param string $Hostname The host, as either a hostname or IP, or as computername\\instance.
     * @param string $Username The username.
     * @param string $Password The password.
     * @param string $Database The database, or an empty string for the default database.
     *
     * @note When specifying $Hostname in the form hostname\\instance, use a
     *       double back-slash if using double quotes.
     * @note Use SQLSrv::LastErrors() to determine whether the connection succeeded.
     * @todo ReturnDateAsStrings could be set to TRUE.
     */
    public function __construct( $Hostname,$Username,$Password,$Database = '' )
    {
        $this->DB = sqlsrv_connect($Hostname,array('UID'=>$Username,'Pwd'=>$Password,'Database'=>$Database));
    }

    /**
     * Execute a query.
     *
     * This method executes a raw SQL statement and expects a correctly
     * formed statement and parameter array.
     *
     * @param string $SQL The SQL statement.
     * @param array $Params The parameter array.
     * @throws Exception Query Error:
     * @retval resource sqlsrv statement resource.
     * @retval boolean FALSE if the query failed.
     *
     * @note Use SQLSrv::LastErrors() to determine the errors if FALSE is returned.
     * @todo Possibly add debugging awareness to this.
     */
    public function Query( $SQL,$Params = array() )
    {
        return sqlsrv_query($this->DB,$SQL,$Params);
    }

    /**
     * Return errors and warnings from the last operation.
     *
     * By default, a string of errors is returned, each on it's own line.  Each
     * error is constructed as:
     *   Error Message (SQLSTATE:Code)
     *
     * Setting $AsArray to TRUE will return an array of strings, indexed by
     * it's error code integer value.
     *
     * @param boolean $AsArray TRUE to return as an array, indexed by SQLSTATE.
     * @param boolean $Warnings TRUE to return warnings in addition to errors.
     * @retval array Array of error strings.
     * @retval NULL No errors or warnings exist.
     *
     * @note By default, this only returns true errors (not warnings).
     * @note sqlsrv_errors() returns duplicate errors for some reason, so we
     *       overwrite based on the error code.
     */
    public function LastErrors( $AsArray = FALSE,$Warnings = FALSE )
    {
        if( ($Errors = sqlsrv_errors($Warnings===TRUE?SQLSRV_ERR_ALL:SQLSRV_ERR_ERRORS)) !== NULL )
        {
            $ErrArr = array();
            foreach( $Errors as $E )
                $ErrArr[$E[1]] = "{$E[2]} ({$E[0]}:{$E[1]})";

            return $AsArray===TRUE?$ErrArr:implode("\r\n",$ErrArr);
        }
        else
            return NULL;
    }


    /*** Implement CRUDC ***/

    /**
     * INSERT a row into a table.
     *
     * @param string $Table The table to insert into.
     * @param array $Values Column names/values for the SET clause.
     * @throws Exception CREATE Error: (includes the error code of the first error)
     * @retval integer The identity value of the inserted row.
     * @retval boolean TRUE if the query was executed but did not create an identity value.
     */
    public function CREATE( $Table,$Values )
    {
        $CP = $this->ToPrepared($Table,$Values,',',TRUE);

        $SQL = "INSERT INTO $Table ({$CP[1]}) VALUES ({$CP[2]}); SELECT SCOPE_IDENTITY();";

        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('CREATE',$Table,$SQL);

        if( ($R = sqlsrv_query($this->DB,$SQL,$CP[0])) === FALSE )
        {
            $E = $this->LastErrors(TRUE);
            throw new Exception("CREATE Error: ".implode("\r\n",$E),key($E));
        }
        else if( ($E = sqlsrv_errors()) !== NULL )
        {
            Log::Log('CREATE Warning: '.print_r($E,TRUE),'WARN');
        }

        sqlsrv_next_result($R);
        return ($I = sqlsrv_fetch_array($R,SQLSRV_FETCH_NUMERIC)[0]) !== NULL?$I:TRUE;
    }

    /**
     * SELECT rows from a table.
     *
     * @param string $Table The table to select from.
     * @param array $Constraint Column name/values that are 'AND' together and used as the WHERE constraint.
     * @param array $Columns Column names or column names/alias to SELECT.
     * @param array $OrderBy Column names/directions (TRUE = ASC, FALSE = DESC).
     * @throws Exception READ Error: (includes the error code of the first error)
     * @retval array An array of rows, each an associative array, or an empty array.
     *
     * @note An empty constraint is allowed - all rows can be SELECT'd.
     * @note This uses client buffered cursors.
     */
    public function READ( $Table,$Constraint = NULL,$Columns = NULL,$OrderBy = NULL )
    {
        if( empty($Columns) )
            $SQL = "SELECT * FROM $Table ";
        else
            $SQL = 'SELECT '.SQL::ToColumnList($Columns)." FROM $Table ";

        $ConstraintParams = array();
        if( !empty($Constraint) )
        {
            $Constraint = $this->ToPrepared($Table,$Constraint,'AND');
            $SQL .= " WHERE {$Constraint[1]} ";
        }

        if( !empty($OrderBy) )
            $SQL .= ' ORDER BY '.SQL::ToOrderBy($OrderBy);

        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('READ',$Table,$SQL);

        if( ($R = sqlsrv_query($this->DB,$SQL,empty($Constraint[0])?array():$Constraint,array('Scrollable'=>'buffered'))) === FALSE )
        {
            throw new Exception('READ Error: '.$this->LastErrors());
        }
        else
        {
            $Rows = array();
            while( ($Row = sqlsrv_fetch_array($R,SQLSRV_FETCH_ASSOC)) !== NULL )
                $Rows[] = $Row;

            return $Rows;
        }
    }

    /**
     * UPDATE rows in a table.
     *
     * @param string $Table The table to update.
     * @param array $Values Column names/values for the SET clause.
     * @param array $Constraint Column name/values that are 'AND' together and used as the WHERE constraint.
     * @throws Exception UPDATE Error: (includes the error code of the first error)
     * @throws Exception UPDATE Empty constraint not allowed.
     * @retval integer The number of affected rows.
     *
     * @note An empty constraint is not allowed.
     */
    public function UPDATE( $Table,$Values,$Constraint )
    {
        $Constraint = $this->ToPrepared($Table,$Constraint,'AND');
        if( empty($Constraint[0]) )
            throw new Exception('UPDATE Empty constraint not allowed.');

        $Values = $this->ToPrepared($Table,$Values);
        $SQL = "UPDATE $Table SET {$Values[1]} WHERE {$Constraint[1]}";

        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('UPDATE',$Table,$SQL);

        if( ($R = sqlsrv_query($this->DB,$SQL,array_merge($Values[0],$Constraint[0]))) === FALSE )
            throw new Exception('UPDATE Error: '.$this->LastErrors());
        else
            return sqlsrv_rows_affected($R);
    }

    /**
     * DELETE rows from a table.
     *
     * @param string $Table The table to delete from.
     * @param array $Constraint Column name/values that are 'AND' together and used as the WHERE constraint.
     * @throws Exception DELETE Error: (includes the error code of the first error)
     * @throws Exception DELETE Empty constraint not allowed.
     * @retval integer The number of affected rows.
     *
     * @note An empty constraint is not allowed.
     */
    public function DELETE( $Table,$Constraint )
    {
        $Constraint = $this->ToPrepared($Table,$Constraint,'AND');
        if( empty($Constraint[0]) )
            throw new Exception('DELETE Empty constraint not allowed.');

        $SQL = "DELETE FROM $Table WHERE {$Constraint[1]}";

        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('DELETE',$Table,$SQL);

        if( ($R = sqlsrv_query($this->DB,$SQL,$Constraint[0])) === FALSE )
            throw new Exception('DELETE Error: '.$this->LastErrors());
        else
            return sqlsrv_rows_affected($R);
    }

    /**
     * COUNT rows in a table.
     *
     * @param string $Table The table to count rows in.
     * @param array $Constraint Column name/values that are 'AND' together and used as the WHERE constraint.
     * @throws Exception COUNT Error: (includes the error code of the first error)
     * @retval integer The count of rows.
     * @retval NULL The count could not be determined.
     *
     * @note An empty constraint is allowed - all rows can be COUNT'd.
     */
    public function COUNT( $Table,$Constraint = NULL )
    {
        $SQL = "SELECT COUNT(*) FROM $Table ";

        if( !empty($Constraint) )
        {
            $Constraint = $this->ToPrepared($Table,$Constraint,'AND');
            $SQL .= " WHERE {$Constraint[1]}";
        }

        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('COUNT',$Table,$SQL);

        if( ($R = sqlsrv_query($this->DB,$SQL,empty($Constraint[0])?array():$Constraint,array('Scrollable'=>'buffered'))) === FALSE )
        {
            throw new Exception('COUNT Error: '.$this->LastErrors());
        }
        else
        {
            return sqlsrv_fetch_array($R,SQLSRV_FETCH_NUMERIC)[0];
        }
    }

    /**
     * Collect column name, type and length information for a table.
     *
     * @param string $Table The table to collect column information for.
     * @throws Exception ColumnInfo Error:
     * @retval array Information for each column in a table.
     *
     * @note This caches the column information between calls.
     */
    public function ColumnInfo( $Table )
    {
        if( isset($this->ColumnInfoCache[$Table]) )
            return $this->ColumnInfoCache[$Table];

        $SQL = 'SELECT COLUMN_NAME,DATA_TYPE,CHARACTER_MAXIMUM_LENGTH
                  FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME=?';

        $R = sqlsrv_query($this->DB,$SQL,array($Table));

        if( $R === FALSE )
        {
            throw new Exception('ColumnInfo Error: '.$this->LastErrors());
        }
        else
        {
            $Info = array();
            while( ($Row = sqlsrv_fetch_array($R,SQLSRV_FETCH_NUMERIC)) !== NULL )
                $Info[$Row[0]] = $Row;

            return ($this->ColumnInfoCache[$Table] = $Info);
        }
    }

    /**
     * Map the columns of a table to an array of SQL typed data.
     *
     * Map() reads the column names of a table and matches them to the elements
     * of an array.  If a column name doesn't exist in the array, it is skipped.
     *
     * An associative array of column name/element name overrides can also be
     * provided to map columns to an element of a different name.  If the column
     * name is mapped to NULL, or to an element that doesn't exist, it is skipped.
     *
     * @param string $Table The table to map.
     * @param array $Src Associative array of data to map.
     * @param array $Override Array of column names/element names for custom mapping.
     * @retval array An associative array of column names and their values.
     *
     * @note This causes one additional SELECT query for every invocation.
     * @note If a column name is mapped to an element that contains an empty string,
     *       NULL, an array, boolean, etc., the returned array will have that value
     *       mapped as well.  This is important to remember regarding default column
     *       values, non-required form fields, multi-selects (i.e. arrays), etc.
     * @note The $Table name is not escaped.
     *
     * @see ColumnInfo()
     */
    public function Map( $Table,$Src,$Override = array() )
    {
        foreach( $this->ColumnInfo($Table) as $I )
        {
            // we have an override (either NULL or a different element name)
            if( array_key_exists($I[0],$Override) )
                $Name = $Override[$I[0]];
            // no override - column name should match element name
            else
                $Name = $I[0];

            // NULL override, skip column
            if( empty($Name) )
                continue;
            // column maps to element, map data
            else if( array_key_exists($Name,$Src) )
                $Dest[$I[0]] = $Src[$Name];
            // column doesn't map to data, skip
            else
                continue;
        }

        return $Dest;
    }

    /**
     * Convert an array of column names/values to an array suitable for prepared statements.
     *
     * ToPrepared() forms a three element numerically indexed array:
     *  - 0: Array of value parameters.
     *  - 1: Column names and placeholders (for WHERE constraints) or column names
     *       (for INSERT) each separated by $Separator.
     *  - 2: Placeholders (?) separated by commas or an empty string.
     *
     * To correctly form the first element, the table's column types are read.
     * Currently, only a type of varbinary is mapped to it's corresponding SQL
     * type (binary stream) to support file uploads.
     *
     * The contents of the second and third elements are determined by the
     * $TwoStrings parameter.  The third element is populated only if $TwoStrings = TRUE.
     *
     * If a value is prefixed with a pipe (|) it is considered a function
     * and is untouched and not included in the parameter array.
     *
     * A PHP value of NULL is translated to a SQL value of NULL.
     *
     * @param string $Table The table to prepare for.
     * @param array $Src An associative array of column names/values.
     * @param string $Separator Separator between each pair or a comma by default.
     * @param boolean $TwoStrings TRUE to populate two strings; one of column names
     *        and one of placeholders.
     * @retval array Three element numeric array as described.
     *
     * @note Any non-string column names are silently skipped if $TwoStrings = FALSE.
     * @note An empty first or second element of the returned array indicates no constraint
     *       and should be checked for.
     *
     * @todo Shouldn't $Separator always be a comma for the third element?
     */
    public function ToPrepared( $Table,$Src,$Separator = ',',$TwoStrings = FALSE )
    {
        // Parameters, column/placeholders or columns, placeholders or empty
        $Ret = array(array(),'','');
        $i = 0;
        $Separator = " $Separator ";
        $CI = $this->ColumnInfo($Table);
        foreach( $Src as $K => $V )
        {
            if( is_string($K) === TRUE )
            {
                // function
                if( !is_object($V) && isset($V{0}) && $V{0} === '|' )
                {
                    if( $TwoStrings )
                    {
                        $Ret[1] .= ($i>0?$Separator:'').$K;
                        $Ret[2] .= ($i++>0?$Separator:'').substr($V,1);
                    }
                    else
                    {
                        $Ret[1] .= ($i++>0?$Separator:'')."{$K}=".substr($V,1);
                    }
                }
                else
                {
                    if( $TwoStrings )
                    {
                        $Ret[1] .= ($i>0?$Separator:'').$K;
                        $Ret[2] .= ($i++>0?$Separator:'').($V===NULL?'NULL':'?');
                    }
                    else
                    {
                        $Ret[1] .= ($i++>0?$Separator:'')."{$K}=".($V===NULL?'NULL':'?');
                    }

                    $Ret[0][] = $this->ToParamType($V,isset($CI[$K])?$CI[$K][1]:'');
                }
            }
        }

        return $Ret;
    }

    /**
     * Form a SQL Server parameter type based on a column type.
     *
     * The following column types are currently supported:
     *  - varbinary: binary stream (for file uploads).
     *
     * Other column types are left alone and will be cast by SQL
     * Server using it's default behavior.
     *
     * @param scalar $Value The value of the column to be typed.
     * @param string $Type The type to convert to (currently only varbinary).
     * @retval array Properly formed SQL server parameter for binary stream.
     * @retval scalar $Value is returned without change.
     */
    public function ToParamType( $Value,$Type = '' )
    {
        if( $Type === 'varbinary' )
            return array($Value,SQLSRV_PARAM_IN,SQLSRV_PHPTYPE_STREAM(SQLSRV_ENC_BINARY),SQLSRV_SQLTYPE_VARBINARY('max'));
        else
            return $Value;
    }

    /**
     * Helper for displaying debug information about the last CRUDC query.
     *
     * @param string $Type Type of CRUDC operation.
     * @param string $Table Table being executed against.
     * @param string $SQL SQL query to be executed.
     */
    protected function DebugCRUDC( $Type,$Table,$SQL )
    {
        $BT = Debug::Backtrace();
        $BT = current(Debug::BT2Str($BT));

        Log::Log("{$Type}('{$Table}') at {$BT}",'LOG',NULL,$SQL);
    }
}


/**
 *  A set of SQL Server SQL templates and tools for executing statements.
 *
 *  SQLSrvSet manages and renders a set of SQL templates into statements
 *  which are executed using a SQLSrv object.
 *
 * @see TemplateSet
 * @see SQLSrv
 */
class SQLSrvSet extends TemplateSet
{
    /**
     * The SQLSrv object to execute statements against.
     */
    protected $DB;

    /**
     * Currently prepared parameters, reset on each __call() call.
     */
    protected $Params = array();


    /**
     * Create a SQLSrvSet object.
     *
     * @param SQLSrv $SQLSrv The SQLSrv object that will be used for executing statements.
     *
     * @note The SQLSrv object is Connect()'d as $DB.
     */
    public function __construct( \fw\SQLSrv $SQLSrv )
    {
        $this->DB = $SQLSrv;
        $this->Connect($this->DB,'DB');
    }

    /**
     * Convert a scalar or an array of scalars to SQL placeholders and parameters.
     *
     * Each scalar will result in a placeholder (?) being return.  An array
     * of scalars will be converted to a placeholder listing, each separated
     * by $Separator.
     *
     * A PHP value of NULL is translated to a SQL value of NULL.
     *
     * This can generate SQL strings that are useful, for example, in functions
     * like IN().
     *
     * @param scalar $Mixed Scalar to be escaped and surrounded.
     * @param array|Traversible $Mixed Set of scalars to convert to a value listing.
     * @param string $Type Explicit column type for given value(s).
     * @param string $Separator Optional separator between each value.  Defaults to a comma.
     * @retval string Escaped SQL value or list of values, or an empty string.
     */
    public function __invoke( $Mixed,$Type = '',$Separator = ',' )
    {
        if( is_array($Mixed) || $Mixed instanceof Traversible )
        {
            foreach( $Mixed as $V )
                $this->Params[] = $this->DB->ToParamType($V,$Type);

            return implode($Separator,array_fill(0,count($Mixed),'?'));
        }
        else if( $Mixed === NULL )
        {
            return 'NULL';
        }
        else if( is_scalar($Mixed) === TRUE )
        {
            $this->Params[] = $this->DB->ToParamType($Mixed,$Type);
            return '?';
        }
        else
            return '';
    }

    /**
     * Render and execute an SQL statement that exists in this SQLSrvSet.
     *
     * @param string $Name Name of the SQL statement to render and execute.
     * @param array $Args Optional associative array of keys/values variables for the statement's scope.
     * @throws Exception $Name not found.
     * @throws Exception $Name Failed:
     * @retval array An array of rows, each an associative array, or an empty array, for a SELECT query.
     * @retval integer The number of affected rows for INSERT, UPDATE and DELETE queries.
     * @retval boolean TRUE if another type of query succeeded, for example a DDL query.
     *
     * @note Statement templates are prepared.
     * @note Variables $RenderingTemplate, $DB (SQLSrv object), and $this are reserved variable names.
     * @note SQLSrvSet does not support ReMap() nor a pre-render Routine.
     * @note This DOES NOT use client buffered cursors (break affected rows).
     * @note SQLSrvSet::$Params is cleared when this method is called.
     *
     * @see Render() to generate and return the SQL statement without executing it.
     * @todo Review return behavior; this may break if multiple statements are executed.
     * @todo Determine how we can tell whether multiple statements were executed - currently nothing is done.
     */
    public function __call( $Name,$Args )
    {
        if( isset($_SERVER[$this->DebugToken]) )
        {
            if( empty($this->Templates[$Name]) )
                throw new Exception("{$Name} not found.");

            $this->Params = array();

            $RenderingTemplate = $this->Templates[$Name];

            extract(Is::Arr(0,$Args)?$this->Connected+$Args[0]:$this->Connected);
            $E = Is::Arr(0,$Args)?$this->Connected+$Args[0]:$this->Connected;
            $this->Extracts = array();
            foreach( $E as $K => $V )
            {
                $this->Extracts[$K] = Is::What($V);
                if( $this->Extracts[$K] === 'object' )
                    $this->Extracts[$K] = get_class($V);

                // strlen() probably not bullet proof but false binary detection otherwise
                if( is_string($V) && strlen($V) > 100 )
                {
                    $finfo = new \finfo(FILEINFO_MIME);
                    $MIMEType = $finfo->buffer($V);
                    if( strpos($MIMEType,'text') === FALSE || strpos($MIMEType,'binary') !== FALSE )
                        ${$K} = "BLOB: $MIMEType";
                    else if( strlen($V) > 500 )
                        ${$K} = 'LONG-STRING';
                }
            }
            unset($E);

            $BT = Debug::Backtrace();
            $BT = current(Debug::BT2Str($BT,'__call'));

            $Buf = "{$Name} called from {$BT}";

            ob_start();
            eval("?>".trim($RenderingTemplate['Body']));
            $DebugSQL = trim(ob_get_clean());
        }

        $this->Params = array();
        if( isset($this->Templates[$Name]) )
        {
            $RenderingTemplate = $this->Templates[$Name];

            extract(Is::Arr(0,$Args)?$this->Connected+$Args[0]:$this->Connected);

            ob_start();
            eval("?>{$RenderingTemplate['Body']}");
            $R = sqlsrv_query($this->DB->DB,ob_get_clean(),$this->Params);

            if( $R === FALSE )
            {
                if( isset($_SERVER[$this->DebugToken]) )
                    Log::Log("{$Buf}\r\n{$DebugSQL}");

                throw new Exception("{$Name} Failed: ".$this->DB->LastErrors());
            }

            // sqlsrv bug - if sqlsrv_has_rows() called twice, first row of result set disappears
            // sqlsrv_num_fields is probably better anyway
            // $HasRows = sqlsrv_has_rows($R);

            if( isset($_SERVER[$this->DebugToken]) )
            {
                $Buf2 = array();
                if( ($NF = sqlsrv_num_fields($R)) )
                {
                    $Buf2['num_fields'] = $NF;
                }
                else
                {
                    $AR = sqlsrv_rows_affected($R);
                    if( $AR === -1 )
                        $Buf2['returning rows_affected'] = '-1 returning TRUE';
                    else if( $AR === FALSE )
                        $Buf2['returning rows_affected'] = 'FALSE';
                    else
                        $Buf2['returning rows_affected'] = $AR;
                }

                $Buf2['Query'] = $DebugSQL;
                $Buf2['Scope'] = $this->Extracts;
                Log::Log($Buf,'LOG',NULL,$Buf2);
            }

            // SELECT - returns all rows as associative array
            if( sqlsrv_num_fields($R) !== 0 )
            {
                $Rows = array();
                while( ($Row = sqlsrv_fetch_array($R,SQLSRV_FETCH_ASSOC)) !== NULL )
                    $Rows[] = $Row;

                return $Rows;
            }
            else
            {
                $C = sqlsrv_rows_affected($R);
                return $C===-1?TRUE:$C;
            }
        }
        else
            throw new Exception("{$Name} not found.");
    }

    /**
     * Render a SQL statement without executing it.
     *
     * @param string $Name Name of the SQL statement to render.
     * @param boolean $Return FALSE for direct output.
     * @throws Exception $Name not found.
     * @retval string The rendered SQL statement.
     *
     * @note Passing arguments is not supported.
     */
    public function Render( $Name,$Return = TRUE )
    {
        if( isset($this->Templates[$Name]) )
        {
            $RenderingTemplate = $this->Templates[$Name];

            extract($this->Connected);

            ob_start();
            eval("?>{$RenderingTemplate['Body']}");
            if( $Return === TRUE )
                return ob_get_clean();
            else
                echo ob_get_clean();
        }
        else
            throw new Exception("{$Name} not found.");
    }

    /**
     * Output information about the prepared parameters for a SQL statement.
     *
     * This method must be called after a statement has been rendered.
     *
     * The variables displayed have been bound with __invoke().
     */
    public function ShowParams()
    {
        $Msg = 'PARAMS:';
        if( empty($this->Params) )
        {
            $Msg .= " <i>none</i>";
        }
        else
        {
            foreach( $this->Params as $K => $V )
                $Msg = "\r\n <i><b>$K</b></i>=".(is_scalar($V)?$V:gettype($V));
        }
        echo Log::Log($Msg);
        return TRUE;
    }

    /**
     * Not implemented.
     */
    public function ReMap( $Name,$DestName )
    {
        throw new Exception("SQLSrvSet does not support ReMap() for '$Name'");
    }

    /**
     * Not implemented.
     */
    public function ApplyDirective( $Key,$Value )
    {
        Log::Log("ApplyDirective not implemented for SQLSrvSet for Key '$Key'");
        return NULL;
    }
}

