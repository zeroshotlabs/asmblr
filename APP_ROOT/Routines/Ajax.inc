<?php
/**
 * @file Ajax.inc Boilerplate ajaxy Routines
 *
 * This library provides Structs and Routines for Ajaxy requests and
 * responses.
 *
 * These Routines are tied to two URL hierarchies, /ajf/ and /json/,
 * which serve HTML fragments and JSON arrays, respectively.
 */


/**
 * An XHR request from jQuery.
 */
abstract class jQueryRequest extends \fw\Struct
{
    protected static $JSON = FALSE;

    /**
     * Initialize a request array.
     *
     * @retval array jQueryRequest Struct array.
     */
    public static function Init()
    {
        if( static::$JSON === FALSE )
        {
            // jQuery sends data as POST by default
            static::$JSON = $_POST;

            // For other cases, such as RESTful APIs, it may be useful to
            // read the raw request directly from stdin.
            // static::$JSON = json_decode(file_get_contents('php://input'),TRUE);
        }

        return static::$JSON;
    }
}


/**
 * A JSON response array.
 */
abstract class JSONResponse extends \fw\Struct
{
    protected static $Skel = array('Method'=>'','Status'=>TRUE,'Msg'=>'','Data'=>'');

    /**
     * Initialize a successful response.
     *
     * @param mixed $Data Payload to include in response.
     * @retval array Response.
     */
    public static function Ok( $Data )
    {
        $R = static::$Skel;
        $R['Method'] = \fw\Path::ToURLString(fw()->Request['Path']);
        $R['Data'] = $Data;
        return $R;
    }

    /**
     * Initialize a failed response.
     *
     * @param string $Msg Error message.
     * @retval array Response.
     */
    public static function Fail( $Msg )
    {
        $R = static::$Skel;
        $R['Method'] = \fw\Path::ToURLString(fw()->Request['Path']);
        $R['Status'] = FALSE;
        $R['Msg'] = $Msg;
        return $R;
    }
}


/**
 * Handle JSON Ajax requests.
 *
 * These methods use data from jQueryRequest and the last path
 * segment of the requested URL to execute a Routine.  They
 * respond with a JSON array (ajf/JSONResponse.tpl).
 */
abstract class JSON extends \fw\RoutineSet
{
    // jQueryRequest and JSONResponse Structs
    public static $Request;
    public static $Response;


    // Page Name: JSON
    //  Page URL: /json/  (hierarchy)
    public static function Execute()
    {
        static::$Request = jQueryRequest::Init();

        $html = fw('html');

        $html->ReMap('Base','ajf_JSONResponse');

        \fw\HTTP::ContentType('json');

        if( empty(static::$Request) )
        {
            $R = JSONResponse::Fail('Empty request');
            fw('html')->Connect($R,'Response');
            return;
        }

        $F = \fw\Path::Bottom(fw()->MatchPath);

        static::{$F}();

        if( !empty(static::$Response) )
            $html->Connect(static::$Response,'Response');
    }

    public static function __callStatic( $Name,$Args )
    {
        \fw\HTTP::_404();
        fw('html')->ReMap('Base',NULL);
    }

    protected static function create_site()
    {

        if( empty($_SESSION['Authorized']) )
            \fw\HTTP::_401();

        $REQ = static::$Request;
        $vr = fw('vr');

        // Validation can proceed similarly to how regular forms are handled
        // (see Internal::MainForm).  The difference here is that we're using
        // the $REQ array as input, which is a jQueryRequest array.
        //
        // For connecting to Mongo, only the database is required.
        $vr->Hostname = \fw\enUS::Text('Hostname',$REQ,FALSE);
        $vr->Username = \fw\enUS::Text('Username',$REQ,FALSE);
        $vr->Password = \fw\enUS::Text('Password',$REQ,FALSE);
        $vr->Database = \fw\enUS::Text('Database',$REQ);

        // If the request appears valid, continue with the connection process
        if( $vr() )
        {
            // Initialize a local array with the connection information.
            $I = array('Hostname'=>$REQ['Hostname'],'Username'=>$REQ['Username'],
                       'Password'=>$REQ['Password'],'Database'=>$REQ['Database']);

            try
            {
                // Use an internal routine to actually make the connection
                $mongo = Internal::ConnectMongo($I);

                // Store the connection information in our session.
                $_SESSION['MongoConnect'] = $I;

                // The connection succeeded so send the appropriate JSON response.
                static::$Response = JSONResponse::Ok('Connected.');
            }
            catch( Exception $E )
            {
                // The connection failed so return the error message as part
                // of the JSON response.
                static::$Response = JSONResponse::Fail($E->getMessage());
            }
        }
        // The form didn't validate so send a message back to the user.
        else
            static::$Response = JSONResponse::Fail('Please check the form.');

        // We use jQuery to highlight which fields are valid and which are not.
        // Here we send the validation results back as part of the JSON array
        // response so that jQuery can correctly update the DOM.
        static::$Response['Data'] = json_encode(fw('vr')->Export());
    }

    //     URL Segment: mysql-connect
    // Source Fragment: ajf/db-tabs.tpl @@@mysql-tab-content
    protected static function MySQLConnect()
    {
        // Since MySQLConnect and SQLSrv have similar validation requirements,
        // the validation has been wrapped in another method.
        //
        // see JSON::CheckConnectForm()
        if( ($I = static::CheckConnectForm()) )
        {
            try
            {
                // Proceed similarly as to ConnectMongo().
                list($mysql,$mysqls) = Internal::ConnectMySQL($I);
                $_SESSION['MySQLConnect'] = $I;

                static::$Response = JSONResponse::Ok('Connected.');
            }
            catch( Exception $E )
            {
                static::$Response = JSONResponse::Fail($E->getMessage());
            }
        }
        else
        {
            static::$Response = JSONResponse::Fail('Please check the form.');
        }

        static::$Response['Data'] = json_encode(fw('vr')->Export());
    }

    //     URL Segment: sqlsrv-connect
    // Source Fragment: ajf/db-tabs.tpl @@@sqlsrv-tab-content
    protected static function SQLSrvConnect()
    {
        // Proceed the same as MySQLConnect()

        if( ($I = static::CheckConnectForm()) )
        {
            try
            {
                list($sqlsrv,$sqlsrvs) = Internal::ConnectSQLSrv($I);
                $_SESSION['SQLSrvConnect'] = $I;

                static::$Response = JSONResponse::Ok('Connected.');
            }
            catch( Exception $E )
            {
                static::$Response = JSONResponse::Fail($E->getMessage());
            }
        }
        else
        {
            static::$Response = JSONResponse::Fail('Please check the form');
        }

        static::$Response['Data'] = json_encode(fw('vr')->Export());
    }

    // Internal method for validating forms for connecting to MySQL and SQL
    // Server.
    //
    // Here validation of the jQueryRequest Struct can be accomplished using
    // the same techniques as are used for form validation.
    protected static function CheckConnectForm()
    {
        $REQ = static::$Request;
        $vr = fw('vr');

        $vr->Hostname = \fw\enUS::Text('Hostname',$REQ);
        $vr->Username = \fw\enUS::Text('Username',$REQ);
        $vr->Password = \fw\enUS::Text('Password',$REQ);
        $vr->Database = \fw\enUS::Text('Database',$REQ);

        if( $vr() )
        {
            $I = array('Hostname'=>$REQ['Hostname'],'Username'=>$REQ['Username'],
                       'Password'=>$REQ['Password'],'Database'=>$REQ['Database']);
            return $I;
        }
        else
            return NULL;
    }
}


/**
 * Handle HTML fragment Ajax requests.
 *
 * These methods use $_GET to execute a Routine, and respond with
 * a fragment of HTML (from HTML/ajf/) based on the last path
 * segment of the requested URL.
 */
abstract class AjaxFrags extends \fw\RoutineSet
{
    // Map $_GET parameters to static methods in this class.
    public static $GETMap = array('mongodisco'=>'MongoDisco',
                                  'mongoinitdb'=>'MongoInitDB',
                                  'mongodelete'=>'MongoDelete',
                                  'mongocopy'=>'MongoCopy',

                                  'mysqldisco'=>'MySQLDisco',
                                  'mysqlinitdb'=>'MySQLInitDB',
                                  'mysqldelete'=>'MySQLDelete',
                                  'mysqlcopy'=>'MySQLCopy',

                                  'sqlsrvdisco'=>'SQLSrvDisco',
                                  'sqlsrvinitdb'=>'SQLSrvInitDB',
                                  'sqlsrvdelete'=>'SQLSrvDelete',
                                  'sqlsrvcopy'=>'SQLSrvCopy');


    // Page Name: AjaxFrags
    //  Page URL: /ajf/  (hierarchy)
    public static function Execute()
    {
        $html = fw('html');
        $ps = fw('ps');

        // Use the last path segment of the request to determine
        // which HTML fragment to serve.
        $F = 'ajf_'.\fw\Path::Bottom(fw()->MatchPath);

        // Determine whether it's known in our HTML TemplateSet
        if( isset($html->$F) )
        {
            // Match a routine to the query string of the request.
            // Note that none, one or more Routine could execute.
            foreach( static::$GETMap as $K => $V )
            {
                // If found, execute the local static method to
                // fullfil the request.
                if( !empty($_GET[$K]) )
                    static::$V();
            }

            // ReMap() the requested template to Base, ensuring only
            // it's contents are returned.
            $html->ReMap('Base',$F);
        }
        // The path segment is unknown so it's a 404 and kill rendering
        else
        {
            \fw\HTTP::_404();
            $html->ReMap('Base',NULL);
        }

        // Execution will now return to fwboot.php and processing,
        // including rendering of the Base template to output the HTML
        // fragment, will continue.
    }

    //  $_GET variable: mongodisco
    protected static function MongoDisco()
    {
        // Blank out any connection information in our session
        // and set Mongo as offline.
        $_SESSION['MongoConnect'] = array();
        fw('page')->MongoOnline = FALSE;
    }

    //  $_GET variable: mysqldisco
    protected static function MySQLDisco()
    {
        $_SESSION['MySQLConnect'] = array();
        fw('page')->MySQLOnline = FALSE;
    }

    //  $_GET variable: sqlsrvdisco
    protected static function SQLSrvDisco()
    {
        $_SESSION['SQLSrvConnect'] = array();
        fw('page')->SQLSrvOnline = FALSE;
    }

    //  $_GET variable: mongoinitdb
    protected static function MongoInitDB()
    {
        // Pull in $mongo and determine our database name from our
        // connection information.
        $mongo = fw('mongo');
        $DB = $_SESSION['MongoConnect']['Database'];

        // $mongo corresponds to the database server.
        // $mongodb corresponds to a specific database on that server.

        // Drop any existing database.
        if( isset($mongo->$DB) )
            $mongo->$DB->drop();

        // Re-alias the database name
        $mongodb = $mongo->Alias($DB,'mongodb');

        // Write a dummy record so that Mongo actually persists the database.
        $mongodb->CREATE('Placeholder',array('Hello'=>'World'));

        // Re-wire $mongodb and set the database as existing.
        fw()->Wire($mongodb,'mongodb');
        fw('page')->MongoDBExists = TRUE;
    }

    //  $_GET variable: mysqlinitdb
    protected static function MySQLInitDB()
    {
        // Proceed similarly to MongoInitDB() with the caveats noted below.

        $mysql = fw('mysql');
        $mysqls = fw('mysqls');
        $DB = $_SESSION['MySQLConnect']['Database'];

        // Use native mysqli::query() and manual escaping to perform
        // DDL operations.
        $DB = $mysql->real_escape_string($I['Database']);
        $mysql->query("DROP DATABASE IF EXISTS $DB");
        $mysql->query("CREATE DATABASE $DB");
        $mysql->query("USE $DB");

        // Use $mysqls SQL templates to create the tables.
        // see SQL/MySQL.sql
        $mysqls->CreateProfileTable();
        $mysqls->CreateFileUploadTable();

        fw('page')->MySQLDBExists = TRUE;
    }

    //  $_GET variable: sqlsrvinitdb
    protected static function SQLSrvInitDB()
    {
        // Proceed similarly to MySQLInitDB().  In the case of SQL Server,
        // we only recreate the tables and the database must already exist.

        $sqlsrv = fw('sqlsrv');
        $sqlsrvs = fw('sqlsrvs');
        $DB = $_SESSION['SQLSrvConnect']['Database'];

        // Perform generic queries.
        $sqlsrv->Query("IF OBJECT_ID('Profile') IS NOT NULL DROP TABLE Profile");
        $sqlsrv->Query("IF OBJECT_ID('FileUpload') IS NOT NULL DROP TABLE FileUpload");

        // Use $sqlsrvs SQL templates to create the tables.
        // see SQL/SQLSrv.sql
        $sqlsrvs->CreateProfileTable();
        $sqlsrvs->CreateFileUploadTable();

        fw('page')->SQLSrvTablesExist = TRUE;
    }

    //  $_GET variable: mongodelete
    protected static function MongoDelete()
    {
        // Use MongoDB's CRDUC interface to perform deletes.
        // The mongodelete variable in $_GET is known to exist because it
        // was used by our parent, AjaxFrags::Execute(), to execute this
        // method.  Note that it must be converted to a MongoID object.
        fw('mongodb')->DELETE('Profile',array('_id'=>new MongoId($_GET['mongodelete'])));
        fw('mongodb')->DELETE('FileUpload',array('Profile_id'=>new MongoId($_GET['mongodelete'])));
    }

    //  $_GET variable: mongodelete
    protected static function MySQLDelete()
    {
        // Proceed similarly to MongoDelete().
        fw('mysql')->DELETE('Profile',array('ProfileID'=>$_GET['mysqldelete']));
        fw('mysql')->DELETE('FileUpload',array('R_ProfileID'=>$_GET['mysqldelete']));
    }

    //  $_GET variable: mongodelete
    protected static function SQLSrvDelete()
    {
        // Proceed similarly to MySQLDelete().
        fw('sqlsrv')->DELETE('Profile',array('ProfileID'=>$_GET['sqlsrvdelete']));
        fw('sqlsrv')->DELETE('FileUpload',array('R_ProfileID'=>$_GET['sqlsrvdelete']));
    }

    //  $_GET variable: mongocopy
    protected static function MongoCopy()
    {
        // Use MongoDB's CRUDC interface to perform reads and creates.
        // The mongocopy variable in $_GET is known to exist because it
        // was used by our parent, AjaxFrags::Execute(), to execute this
        // method.  Note that it must be converted to a MongoID object.
        $R = fw('mongodb')->READ('Profile',array('_id'=>new MongoId($_GET['mongocopy'])));

        if( !empty($R) )
        {
            // MongoDB returns results as an associative array, keyed by
            // each record's _id.  Here we grab the first record.
            $R = current($R);

            // Remove the _id so that a new one is created (and force the
            // operation to be a create, rather than an update).
            unset($R['_id']);

            // Create the record.
            fw('mongodb')->CREATE('Profile',$R);
        }
    }

    //  $_GET variable: mysqlcopy
    protected static function MySQLCopy()
    {
        // Proceed similarly to MongoCopy() with the following caveats.
        $R = fw('mysql')->READ('Profile',array('ProfileID'=>$_GET['mysqlcopy']));

        // MySQL returns results as a numeric array of associative arrays.
        if( !empty($R[0]) )
        {
            // Remove the primary key so that a new auto-incremented
            // value will be created.
            unset($R[0]['ProfileID']);

            // Create the record.
            fw('mysql')->CREATE('Profile',$R[0]);
        }
    }

    //  $_GET variable: sqlsrvcopy
    protected static function SQLSrvCopy()
    {
        // Proceed similarly to MySQLCopy().

        $R = fw('sqlsrv')->READ('Profile',array('ProfileID'=>$_GET['sqlsrvcopy']));

        if( !empty($R[0]) )
        {
            unset($R[0]['ProfileID']);
            fw('sqlsrv')->CREATE('Profile',$R[0]);
        }
    }
}

