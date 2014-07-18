<?php
/**
 * @file MySQL.inc MySQL connectivity.
 * @author Stackware, LLC
 * @version 4.0
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


// NEEDS TESTING

/**
 * This extension is composed of two classes:
 *
 *  - The MySQL class which extends MySQLi and implements both Wireable and CRUDC,
 *    and includes debugging, error handling, data mapping, and SQL generation.  The
 *    full native functionality of MySQLi is untouched.
 *
 *  - The MySQLSet class which extends TemplateSet for custom SQL statements.
 *
 * The MySQL extension provides two ways to query the database:
 *
 *  - MySQL/CRUDC: Simple CREATE/READ/UPDATE/DELETE/COUNT queries with automatic
 *    SQL generation and escaping.  Queries are executed using a fixed set of
 *    methods and customization is limited.  Column nor table names are escaped.
 *
 *  - MySQLSet/TemplateSet: Specialized Templates for dynamically generating
 *    custom SQL with complex SQL generation and escaping.  Queries are rendered
 *    and executed by Name.
 */
class MySQL extends \mysqli implements Debuggable,CRUDC
{
    use Debugged;


    /*** Implement CRUDC ***/

    /**
     * INSERT a row into a table.
     *
     * @param string $Table The Table or Database.Table to insert into.
     * @param array $Values Column names/values for the SET clause.
     * @throws Exception CREATE Error:
     * @retval integer The auto-increment value of the inserted row.
     * @retval boolean TRUE if the query was executed but did not create an auto-increment value.
     *
     * @note Warnings generated during query execution are logged with Log::Log().
     * @todo Possibly add ON DUPLICATE KEY awareness.
     */
    public function CREATE( $Table,$Values )
    {
        $SQL = "INSERT INTO $Table SET ".$this->ToColumnValueList($Values);

        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('CREATE',$Table,$SQL);

        if( $this->query($SQL) === FALSE )
        {
            throw new Exception("{$this->WiredAs}::CREATE Error: {$this->error}");
        }
        else if( $this->warning_count === 0 )
        {
            return $this->insert_id===0?TRUE:$this->insert_id;
        }
        else
        {
            Log::Log("CREATE Warning: ".$this->DebugWarnings(),'WARN',NULL,$Values);
            return ((int) Struct::Get(0,$this->query('SELECT LAST_INSERT_ID() FROM DUAL')->fetch_row()));
        }
    }

    /**
     * SELECT rows from a table.
     *
     * @param string $Table The Table or Database.Table to select from.
     * @param array $Constraint Column name/values that are 'AND' together and used as the WHERE constraint.
     * @param array $Columns Column names or column names/alias to SELECT.
     * @param array $OrderBy Column names/directions (TRUE = ASC, FALSE = DESC).
     * @throws Exception READ Error:
     * @retval array An array of rows, each an associative array, or an empty array.
     *
     * @note An empty constraint is allowed - all rows can be SELECT'd.
     */
    public function READ( $Table,$Constraint = NULL,$Columns = NULL,$OrderBy = NULL )
    {
        if( empty($Columns) )
            $SQL = "SELECT * FROM $Table ";
        else
            $SQL = 'SELECT '.SQL::ToColumnList($Columns)." FROM $Table ";

        if( !empty($Constraint) )
            $SQL .= ' WHERE '.$this->ToColumnValueList($Constraint,'AND');

        if( !empty($OrderBy) )
            $SQL .= ' ORDER BY '.SQL::ToOrderBy($OrderBy);

        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('READ',$Table,$SQL);

        if( ($R = $this->query($SQL)) === FALSE )
            throw new Exception("{$this->WiredAs}::READ Error: {$this->error}");
        else
            return $R->fetch_all(MYSQLI_ASSOC);
    }

    /**
     * UPDATE rows in a table.
     *
     * @param string $Table The Table or Database.Table to update.
     * @param array $Values Column names/values for the SET clause.
     * @param array $Constraint Column name/values that are 'AND' together and used as the WHERE constraint.
     * @throws Exception UPDATE Error:
     * @throws Exception UPDATE Empty constraint not allowed.
     * @retval integer The number of affected rows.
     *
     * @note An empty constraint is not allowed.
     */
    public function UPDATE( $Table,$Values,$Constraint )
    {
        $Constraint = $this->ToColumnValueList($Constraint,'AND');
        if( empty($Constraint) )
            throw new Exception('UPDATE Empty constraint not allowed.');

        $SQL = "UPDATE $Table SET ".$this->ToColumnValueList($Values)." WHERE $Constraint";

        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('UPDATE',$Table,$SQL);

        if( $this->query($SQL) === FALSE )
            throw new Exception("UPDATE Error: {$this->error}");
        else
            return $this->affected_rows;
    }

    /**
     * DELETE rows from a table.
     *
     * @param string $Table The Table or Database.Table to delete from.
     * @param array $Constraint Column name/values that are 'AND' together and used as the WHERE constraint.
     * @throws Exception DELETE Error:
     * @throws Exception DELETE Empty constraint not allowed.
     * @retval integer The number of affected rows.
     *
     * @note An empty constraint is not allowed.
     */
    public function DELETE( $Table,$Constraint )
    {
        $Constraint = $this->ToColumnValueList($Constraint,'AND');
        if( empty($Constraint) )
            throw new Exception('DELETE Empty constraint not allowed.');

        $SQL = "DELETE FROM $Table WHERE $Constraint";

        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('DELETE',$Table,$SQL);

        if( $this->query($SQL) === FALSE )
            throw new Exception("DELETE Error: {$this->error}");
        else
            return $this->affected_rows;
    }

    /**
     * COUNT rows in a table.
     *
     * @param string $Table The Table or Database.Table to count rows in.
     * @param array $Constraint Column name/values that are 'AND' together and used as the WHERE constraint.
     * @throws Exception COUNT Error:
     * @retval integer The count of rows.
     * @retval NULL The count could not be determined.
     *
     * @note An empty constraint is allowed - all rows can be COUNT'd.
     */
    public function COUNT( $Table,$Constraint = NULL )
    {
        $SQL = "SELECT COUNT(*) FROM $Table ";

        if( !empty($Constraint) )
            $SQL .= ' WHERE '.$this->ToColumnValueList($Constraint,'AND');

        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('COUNT',$Table,$SQL);

        $R = $this->query($SQL);

        if( $R === FALSE )
        {
            throw new Exception("COUNT Error: {$this->error}");
        }
        else
        {
            $T = $R->fetch_row();
            return isset($T[0])===TRUE?(int)$T[0]:NULL;
        }
    }

    /**
     * Map the columns of a table to an array of data.
     *
     * Map() reads the column names of a table and matches them to the elements
     * of an array.  If a column name doesn't exist in the array, it is skipped.
     *
     * An associative array of column name/element name overrides can also be
     * provided to map columns to an element of a different name.  If the column
     * name is mapped to NULL, or to an element that doesn't exist, it is skipped.
     *
     * @param string $Table The Table or Database.Table to map.
     * @param array $Src Associative array of data to map.
     * @param array $Override Array of column names/element names for custom mapping.
     * @throws Exception Map Unable to determine database from '$Table'.
     * @retval array An associative array of column names and their values.
     *
     * @note This causes at least one additional SELECT query for every invocation.  If
     *       a database name is not provided, an additional query is attempted to
     *       determine the current database.
     * @note If a column name is mapped to an element that contains an empty string,
     *       NULL, an array, boolean, etc., the returned array will have that value
     *       mapped as well.  This is important to remember regarding default column
     *       values, non-required form fields, multi-selects (i.e. arrays), etc.
     */
    public function Map( $Table,$Src,$Override = array() )
    {
        if( strpos($Table,'.') !== FALSE )
        {
            list($Database,$Table) = explode('.',$Table);
        }
        // no database was provided - we'll try to detect
        else
        {
            $T = $this->query('SELECT DATABASE()')->fetch_row();
            if( isset($T[0]) )
                $Database = $T[0];
            else
                throw new Exception("Map Unable to determine database from '$Table'.");
        }

        $SQL = "SELECT column_name FROM information_schema.columns
                 WHERE ".$this->ToColumnValueList(array('table_name'=>$Table,'table_schema'=>$Database),'AND');

        $R = $this->query($SQL);
        $Dest = array();
        foreach( $R->fetch_all() as $K => $V )
        {
            // we have an override (either NULL or a different element name)
            if( array_key_exists($V[0],$Override) )
                $Name = $Override[$V[0]];
            // no override - column name should match element name
            else
                $Name = $V[0];

            // NULL override, skip column
            if( empty($Name) )
                continue;
            // column maps to element, map data
            else if( array_key_exists($Name,$Src) )
                $Dest[$V[0]] = $Src[$Name];
            // column doesn't map to data, skip
            else
                continue;
        }

        return $Dest;
    }

    /**
     * Convert an array of column names/values to a SQL string.
     *
     * ToColumnValueList() forms WHERE constraint clauses and
     * INSERT/UPDATE SET clauses.
     *
     * Each value is properly escaped and enclosed in single quotes unless
     * unless it is prefixed with a pipe (|), in which case it is untouched.
     *
     * A PHP value of NULL is translated to a SQL value of NULL.
     *
     * Column names are not escaped.
     *
     * @param array $Src An associative array of column names/values.
     * @param string $Separator Separator between each pair or comma by default.
     * @retval string SQL string of separated column names and values or an empty string.
     *
     * @note Any non-string column names are silently skipped.
     * @note An empty string return value indicates no constraint, and should be checked for.
     */
    public function ToColumnValueList( $Src,$Separator = ',' )
    {
        $SQL = '';
        $i = 0;
        $Separator = " $Separator ";
        foreach( $Src as $K => $V )
        {
            if( is_string($K) === TRUE )
            {
                $SQL .= ($i++>0?$Separator:'')."`{$K}`=".($V===NULL?'NULL':(isset($V[0])===TRUE&&$V[0]==='|'?substr($V,1):"'".$this->real_escape_string($V)."'"));
            }
        }

        return $SQL;
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

        Log::Log("\${$this->DebugToken}::{$Type}('{$Table}') at {$BT}: {$SQL}",'LOG',NULL);
    }

    /**
     * Return the last database warnings.
     *
     * @retval string Listing of database warnings from last query, one on each line.
     */
    public function DebugWarnings()
    {
        $W = $this->query('SHOW WARNINGS')->fetch_all(MYSQLI_ASSOC);
        if( empty($W) )
            return ' NO MYSQL WARNINGS ';

        $S = '';
        foreach( $W as $R )
            $S .= "\r\n{$R['Level']} {$R['Code']}: {$R['Message']}";

        return $S;
    }
}


/**
 * A set of MySQL SQL templates and tools for executing statements.
 *
 * MySQLSet manages and renders a set of SQL templates into statements
 * which are executed using a MySQL object.
 *
 * @note CRUDC operations must be performed using a \asm\MySQL object, rather than this one.
 *
 * @see TemplateSet
 * @see MySQL
 */
class MySQLSet extends TemplateSet
{
    /**
     * The MySQL object to execute statements against.
     */
    protected $DB;


    /**
     * Create a MySQLSet object.
     *
     * @param MySQL $MySQL The MySQL object that will be used for executing statements.
     * @param string $Path The file containing the SQL statements.
     *
     * @note The MySQL object is Connect()'d as $DB.
     * @todo If we get rid of @@@ we'd need $Path to be treated recursively
     */
    public function __construct( \asm\MySQL $MySQL,$Path = '' )
    {
        if( !empty($Path) )
            $this->LoadFile($Path,$GLOBALS['asmapp']);

        $this->DB = $MySQL;
        $this->Connect(array('DB'=>$this->DB));
    }

    /**
     * Convert a scalar or an array of scalars to a SQL string.
     *
     * Each scalar is properly escaped and surrounded by $Surround.  An array
     * of scalars will be converted to a value listing, each separated
     * by $Separator.
     *
     * A PHP value of NULL is translated to a SQL value of NULL.
     *
     * This can generate SQL strings that are useful, for example, in functions
     * like IN().
     *
     * @param scalar $Mixed Scalar to be escaped and surrounded.
     * @param array|Traversible $Mixed Set of scalars to convert to a value listing.
     * @param string $Surround Optional wrapper for each scalar.  Defaults to the single quote.
     * @param string $Separator Optional separator between each value.  Defaults to a comma.
     * @retval string Escaped SQL value or list of values, or an empty string.
     */
    public function __invoke( $Mixed,$Surround = '\'',$Separator = ',' )
    {
        if( is_array($Mixed) || $Mixed instanceof Traversible )
        {
            $S = '';
            $i = 0;
            foreach( $Mixed as $V )
                $S .= ($i++>0?$Separator:'').($V===NULL?'NULL':($Surround.$this->DB->real_escape_string($V).$Surround));

            return $S;
        }
        else if( $Mixed === NULL )
            return 'NULL';
        else if( is_scalar($Mixed) === TRUE )
            return $Surround.$this->DB->real_escape_string($Mixed).$Surround;
        else
            return '';
    }

    /**
     * Render and execute an SQL statement that exists in this MySQLSet.
     *
     * @param string $Name Name of the SQL statement to render and execute.
     * @param array $Args Optional associative array of keys/values variables for the statement's scope.
     * @throws Exception $Name not found.
     * @throws Exception $Name Failed:
     * @retval array An array of rows, each an associative array, or an empty array, for a SELECT query.
     * @retval integer The last inserted ID for an INSERT query that generates an auto-increment ID.
     * @retval integer The number of affected rows for UPDATE and DELETE queries.
     * @retval boolean TRUE if another type of query succeeded, for example a DDL query, or no rows were changed.
     *
     * @note WARNING: There is no escaping - it should be done inline in the statement.
     * @note Variables $RenderingTemplate, $DB (MySQL object), and $this are reserved variable names.
     * @note MySQLSet does not support ReMap() nor a pre-render Routine.
     * @note If an UPDATE/DELETE doesn't change any rows, TRUE is returned.
     *
     * @see Render() to generate and return the SQL statement without executing it.
     */
    public function __call( $Name,$Args )
    {
        if( isset($_SERVER[$this->DebugToken]) )
            $DebugSQL = $this->Debug__call($Name,$Args);

        if( isset($this->Templates[$Name]) )
        {
            $RenderingTemplate = $this->Templates[$Name];

            // scope the connected variables plus any arguments
            foreach( $this->Connected as $K => $V )
                $$K = $V;

            if( !empty($Args[0]) )
            {
                foreach( $Args[0] as $K => $V )
                    $$K = $V;
            }

            ob_start();
            eval($RenderingTemplate['Body']);
            $R = $this->DB->query(ob_get_clean());

            if( $R === FALSE )
            {
                if( isset($_SERVER[$this->DebugToken]) )
                {
                    $Buf2 = array('DBError'=>$this->DB->error,'Query'=>$DebugSQL,'Scope'=>$this->Extracts);
                    Log::Log($Buf,'ERROR',NULL,$Buf2);
                }

                throw new Exception("Query {$Name} Failed: {$this->DB->error}");
            }

            if( isset($_SERVER[$this->DebugToken]) )
                $this->Debug__call2($R,$DebugSQL,$Args);

            if( is_a($R,'mysqli_result') )
                return $R->fetch_all(MYSQLI_ASSOC);
            else if( $this->DB->insert_id !== 0 )
                return $this->DB->insert_id;
            else if( $this->DB->affected_rows !== 0 )
                return $this->DB->affected_rows;
            else
                return TRUE;
        }
        else
            throw new Exception("MySQL Query '{$Name}' not found.");
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
            throw new Exception("{$this->WiredAs}::{$Name} not found.");
    }

    /**
     * Not implemented.
     */
    public function ReMap( $Name,$DestName )
    {
        throw new Exception("MySQLSet does not support ReMap() for '$Name'");
    }

    /**
     * Internal method for debugging MySQL query rendering.
     *
     * Debug info includes available variable within the scope of rendering.
     *
     * @param string $Name Name of query.
     * @param array $Args Optional render-time arguments.
     */
    protected function Debug__call( $Name,$Args )
    {
        if( empty($this->Templates[$Name]) )
            throw new Exception("{$this->WiredAs}::{$Name} not found.");

        $RenderingTemplate = $this->Templates[$Name];

        extract(Is::Arr(0,$Args)?$this->Connected+$Args[0]:$this->Connected);
        $E = Is::Arr(0,$Args)?$this->Connected+$Args[0]:$this->Connected;

        $Extracts = array();
        foreach( $E as $K => $V )
        {
            $Extracts[$K] = Is::What($V);
            if( $Extracts[$K] === 'object' )
                $Extracts[$K] = get_class($V);

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

        $Buf = "\${$this->DebugToken}::{$Name} called from {$BT}";

        ob_start();
        eval($RenderingTemplate['Body']);
        $DebugSQL = trim(ob_get_clean());

        return $DebugSQL;
    }

    /**
     * @todo Doesn't fully support logging functionality because Log::Sys() doesn't
     */
    protected function Debug__call2( $R,$DebugSQL,$Args )
    {
        $Buf2 = array();
        if( is_a($R,'mysqli_result') === TRUE )
            $Buf2['returning num_rows'] = $R->num_rows;
        else if( $this->DB->insert_id !== 0 )
            $Buf2['returning insert_id'] = $this->DB->insert_id;
        else if( $this->DB->affected_rows !== 0 )
            $Buf2['returning affected_rows'] = $this->DB->affected_rows;
        else
            $Buf2['returning'] = TRUE;

        if( $this->DB->warning_count !== 0 )
        {
            $Buf2['warning_count'] = $this->DB->warning_count;
            $Buf2['Warnings'] = $this->DB->DebugWarnings();
        }

        $Buf2['Query'] = $DebugSQL;
        $Buf2['Scope'] = Is::Arr(0,$Args)?$this->Connected+$Args[0]:$this->Connected;

        Log::Log($DebugSQL,'LOG',NULL,$Buf2);
    }
}

