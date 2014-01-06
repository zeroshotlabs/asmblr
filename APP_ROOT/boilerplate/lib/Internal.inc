<?php
/**
 * @file Util.inc Boilerplate internal Routines
 *
 * These are internal Routines and not meant to be tied to
 * a Page/URL.
 *
 * see Routines/Request.inc for how these are used.
 */

abstract class Internal
{
    // Load and connect to all three databases.
    // called from fwApp::Go()
    public static function StartupDB()
    {
        // Load the extensions for each database.
        \fw\Inc::Ext('Mongo.inc');
        \fw\Inc::Ext('MySQL.inc');
        \fw\Inc::Ext('SQLSrv.inc');

        // Connect database if credentials are available.  Each of these
        // may throw an exception, which will bubble up as a fatal 500
        // server error.
        if( !empty($_SESSION['MongoConnect']) )
            static::ConnectMongo($_SESSION['MongoConnect']);

        if( !empty($_SESSION['MySQLConnect']) )
            static::ConnectMySQL($_SESSION['MySQLConnect']);

        if( !empty($_SESSION['SQLSrvConnect']) )
            static::ConnectSQLSrv($_SESSION['SQLSrvConnect']);
    }

    // Connect to a Mongo server using provided credentials.
    public static function ConnectMongo( &$I )
    {
        // Fine tune the connection details for Mongo.
        $I['Hostname'] = empty($I['Hostname'])?'localhost':$I['Hostname'];
        if( !empty($I['Username']) )
        {
            $I2['username'] = $I['Username'];
            $I2['password'] = $I['Password'];
        }
        else
            $I2 = array();

        // Framewire's Mongo extension provides two classes, Mongo and MongoDB,
        // each of which extends PHP's native MongoClient and MongoDB classes,
        // respectively.  Mongo represents a connection to the server, while
        // MongoDB represents a specific database.
        //
        // Here we instaniate a Mongo object, wire it, and flag that
        // a Mongo connection is available.
        $mongo = new \fw\Mongo("mongodb://{$I['Hostname']}",$I2);
        fw()->Wire($mongo,'mongo');
        fw('page')->MongoOnline = TRUE;

        // We're able to connect to the server but we don't know if the
        // database exists.
        //
        // Here we use a native Mongo method, listDBs(), to determine and
        // flag whether the database we want to use already exists.
        foreach( $mongo->listDBs()['databases'] as $D )
        {
            if( $D['name'] === $I['Database'] )
            {
                fw('page')->MongoDBExists = TRUE;
                break;
            }
        }

        // The database exists.
        if( fw('page')->MongoDBExists === TRUE )
        {
            // Framewire's Mongo extension requires that a database name
            // be explicitly Alias()'d, which avoids the unexpected creation
            // of a database because of a typo.
            //
            // Here we alias the database name to a generic label, mongodb,
            // which is also wired for global use.
            $mongodb = $mongo->Alias($I['Database'],'mongodb');
            fw()->Wire($mongodb,'mongodb');
        }
        else
            fw('page')->MongoDBExists = FALSE;

        return $mongo;
    }

    // Connect to a MySQL server using provided credentials.
    public static function ConnectMySQL( $I )
    {
        // Framewire's MySQL extension provides two classes, MySQL and
        // MySQLSet.
        //
        // MySQL extends PHP's native mysqli, allowing it's full functionality
        // to be available, while adding features such as debugging, error
        // handling and the implementation of the CRUDC interface.

        // Here we instantiate a MySQL object and check if the connection
        // had any errors.
        $mysql = new \fw\MySQL($I['Hostname'],$I['Username'],$I['Password']);

        if( $mysql->connect_errno !== 0 )
            throw new Exception('MySQL: '.$mysql->connect_error);

        // The MySQL extension also provides MySQLSet, which extends
        // TemplateSet.  It's used to manage and generate custom SQL queries
        // from SQL "templates."
        //
        // MySQLSet dynamically generates complex statement - akin to how
        // HTML templates are generated - and provides tools for escaping
        // data, error handling, debugging, and execution.
        //
        // Here we instantiate a MySQLSet, which requires a MySQL object, and
        // load SQL statement templates from disk (akin to enUSHTMLSet).
        $mysqls = new \fw\MySQLSet($mysql);
        $mysqls->LoadFile('SQL/MySQL.sql');

        // Wire both objects and indicate that MySQL is online.
        fw()->Wire(array('mysql'=>$mysql,'mysqls'=>$mysqls));
        fw('page')->MySQLOnline = TRUE;

        // We're able to connect to the database but we don't know if the
        // database exists.
        //
        // Here we use a native mysqli method, query(), to determine and
        // flag whether the database we want to use already exists.
        $DB = $mysql->real_escape_string($I['Database']);
        fw('page')->MySQLDBExists = $mysql->query("USE $DB");

        return array($mysql,$mysqls);
    }

    // Connect to a SQL Server using provided credentials.
    public static function ConnectSQLSrv( $I )
    {
        // Framewire's SQLSrv extension provides two classes, SQLSrv and
        // SQLSrvSet.
        //
        // SQLSrv wraps PHP's native sqlsrv family of functions, adding
        // adding features such as debugging, error handling and the
        // implementation of the CRUDC interface.

        // Here we instantiate a SQLSrv object and check if the connection
        // had any errors.
        $sqlsrv = new \fw\SQLSrv($I['Hostname'],$I['Username'],$I['Password'],$I['Database']);

        if( ($E = $sqlsrv->LastErrors()) !== NULL )
            throw new Exception("SQLSrv: ".$E);

        // The SQLSrv extension also provides SQLSrvSet, which works similarly
        // to MySQLSet.
        $sqlsrvs = new \fw\SQLSrvSet($sqlsrv);
        $sqlsrvs->LoadFile('SQL/SQLSrv.sql');

        fw()->Wire(array('sqlsrv'=>$sqlsrv,'sqlsrvs'=>$sqlsrvs));
        fw('page')->SQLSrvOnline = TRUE;

        // We're able to connect to the database but we don't know if the
        // tables exists.  In the case of SQL Server, we can't create a
        // database on the fly.
        //
        // Here we use SQLSrv::ColumnInfo() to determine and flag whether
        // the tables exist.
        $PI = $sqlsrv->ColumnInfo('Profile');
        $FUI = $sqlsrv->ColumnInfo('FileUpload');
        fw('page')->SQLSrvTablesExist = !empty($PI) && !empty($FUI);

        return array($sqlsrv,$sqlsrvs);
    }

    // Enable debug toggling based on the $_GET query string.
    //
    // called from fwApp::Go()
    // see HTML/examples/DebugMenu.tpl for use of the $page flags.
    public static function DebugHandler()
    {
        // If LogPublic isn't TRUE, all debug requests are ignored.
        if( fw()->LogPublic !== TRUE )
            return;

        // Enable debugging for the PageSet.
        //
        // If flagged in the query string, and the object exists,
        // enable debugging.
        if( !empty($_GET['psdebug']) && fw('ps') )
        {
            fw('ps')->DebugOn();
            fw('page')->psdebug = TRUE;

            // Propagate this query string parameter so that
            // all created links include it (unless overriden).
            fw('lp')->Set(array('psdebug'=>'1'));
        }

        if( !empty($_GET['htmldebug']) && fw('html') )
        {
            fw('html')->DebugOn();
            fw('lp')->Set(array('htmldebug'=>'1'));
            fw('page')->htmldebug = TRUE;
        }

        if( !empty($_GET['mongodbdebug']) && fw('mongo') && fw('mongodb') )
        {
            fw('lp')->Set('?mongodbdebug=1');
            fw('page')->mongodbdebug = TRUE;
            fw('mongodb')->DebugOn();
            fw('mongo')->DebugOn();
        }

        if( !empty($_GET['mysqldebug']) && fw('mysql') )
        {
            fw('lp')->Set('?mysqldebug=1');
            fw('page')->mysqldebug = TRUE;
            fw('mysql')->DebugOn();
        }

        if( !empty($_GET['mysqlsdebug']) && fw('mysqls') )
        {
            fw('lp')->Set('?mysqlsdebug=1');
            fw('page')->mysqlsdebug = TRUE;
            fw('mysqls')->DebugOn();
        }

        if( !empty($_GET['sqlsrvdebug']) && fw('sqlsrv') )
        {
            fw('lp')->Set('?sqlsrvdebug=1');
            fw('page')->sqlsrvdebug = TRUE;
            fw('sqlsrv')->DebugOn();
        }

        if( !empty($_GET['sqlsrvsdebug']) && fw('sqlsrvs') )
        {
            fw('lp')->Set('?sqlsrvsdebug=1');
            fw('page')->sqlsrvsdebug = TRUE;
            fw('sqlsrvs')->DebugOn();
        }
    }

    // Routine for the example-aside template.
    // see the call to SetRoutine() in fwboot.php
    public static function ExamplesAside()
    {
        if( fw('page')->MySQLDBExists )
        {
            $MP = fw('mysqls')->ListProfiles();
            fw('html')->Connect($MP,'MySQLProfiles');
        }

        if( fw('page')->SQLSrvTablesExist )
        {
            $SP = fw('sqlsrvs')->ListProfiles();
            fw('html')->Connect($SP,'SQLSrvProfiles');
        }

        if( fw('page')->MongoDBExists )
        {
            $FP = fw('mongodb')->READ('Profile');
            fw('html')->Connect($FP,'MongoProfiles');
        }
    }

    // Validate the example form.
    //
    // Framewire provides locale-specific form validation.  \fw\enUS
    // is included by default.
    //
    // This method demonstrates all validations provided by \fw\enUS,
    // and tracking the results using a ValidationReport.
    //
    // called by Request::Examples()
    public static function MainForm()
    {
        // Make our ValidationReport available for validation tracking.
        $vr = fw('vr');

        // NOTE: These methods may change the data passed to them,
        // for example, trim()'ing a text field.

        // Validate a single-select field.  This will ensure the
        // submitted value exists within the pre-defined set,
        // (returned from PrefixListing()), and if required, that a
        // selection has been made.
        $vr->Prefix = \fw\enUS::Single('Prefix',$_POST,\fw\enUS::PrefixListing());

        // Normal text fields - whitespace is trimmed.
        $vr->FirstName = \fw\enUS::Text('FirstName',$_POST);
        $vr->LastName = \fw\enUS::Text('LastName',$_POST);

        // Adding FALSE makes the form field not required.
        $vr->Suffix = \fw\enUS::Single('Suffix',$_POST,\fw\enUS::SuffixListing(),FALSE);

        $vr->Address1 = \fw\enUS::Text('Address1',$_POST);
        $vr->Address2 = \fw\enUS::Text('Address2',$_POST,FALSE);

        $vr->City = \fw\enUS::Text('City',$_POST);
        $vr->State = \fw\enUS::Single('State',$_POST,\fw\enUS::StateListing());

        // Locale specific, US zip code validation.
        $vr->ZipCode = \fw\enUS::ZipCode('ZipCode',$_POST);

        $vr->Email = \fw\enUS::Email('Email',$_POST);
        $vr->Age = \fw\enUS::Number('Age',$_POST);
        $vr->PhoneNumber = \fw\enUS::PhoneNumber('PhoneNumber',$_POST);

        $vr->CCN = \fw\enUS::CCNumber('CCN',$_POST);
        $vr->IP = \fw\enUS::IP('IP',$_POST);
        $vr->SSN = \fw\enUS::SSN('SSN',$_POST);

        // Textareas may contain newlines, whereas text fields cannot.
        $vr->Desc = \fw\enUS::Textarea('Desc',$_POST);

        $vr->Username = \fw\enUS::Username('Username',$_POST);

        // Password includes the option to validate against another field.
        $vr->Password = \fw\enUS::Password('Password',$_POST,'Password2');

        $vr->EmailACopy = \fw\enUS::Email('EmailACopy',$_POST,FALSE);

        // \fw\enUS::Multi() is used when multiple options can be selected.
        // Here it's used for a set of checkboxes.
        $vr->HAU = \fw\enUS::Multi('HAU',$_POST,fw('page')->HAU);

        // If the submitted values are valid, check that at least two values
        // have been submitted.
        if( $vr->HAU )
            $vr->HAU = count($_POST['HAU'])>1?TRUE:FALSE;

        // The \fw\FileUpload Struct is used to manage and validate one
        // or more file uploads.
        //
        // Here we specify the name of the form field that may contain
        // the metadata about the file uploads.
        $Files = \fw\FileUpload::Init('FileUp');

        $vr->FileUp = TRUE;
        foreach( $Files as $F )
        {
            if( \fw\FileUpload::IsOK($F) === FALSE )
            {
                $vr->FileUp = FALSE;
                fw('msg')->FormMsg = 'File upload failed: '.\fw\FileUpload::Err2Txt($F['Error']);
            }
        }
    }
}


