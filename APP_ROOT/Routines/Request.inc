<?php
/**
 * @file Request.inc Boilerplate primary Routines
 *
 * Review DOC_ROOT/fwboot.php first.
 *
 * Each Routine here is connected to a Page and thus handles
 * a request.  The Routine will execute when it's Page's
 * URL matches the request.
 *
 * see DOC_ROOT/fwboot.php for the creation of the Pages.
 */

abstract class Request extends \fw\RoutineSet
{
    // Page Name: Home
    //  Page URL: /
    public static function Home()
    {
        // Template Names are ReMap()'d to swap one Template for another.
        // Then when a name is called for rendering, the mapped-to Template
        // is rendered instead.
        //
        // fw() is a shorthand function for accessing Wire()'d objects by
        // Name.  In this case, 'html' gives us access to the enUSHTMLSet
        // object we create in fwboot.php.
        //
        // Here we retrieve the 'html' object created in fwboot.php and
        // ReMap() Article to the Home template.
        //
        // see APP_ROOT/HTML/Base.tpl
        // see APP_ROOT/HTML/Home.tpl
        fw('html')->ReMap('Article','Home');
    }

    // Page Name: CSS
    //  Page URL: /css/
    public static function CSSHandler()
    {
        // Use Path to grab the last path part of the matching URL
        // and prefix the subdirectory for the CSS templates.
        $F = 'css_'.str_replace('.css','',\fw\Path::Bottom(fw()->MatchPath));

        // Ensure we have a known CSS file.
        if( isset(fw('html')->{$F}) )
        {
            // Send the correct content type and charset headers.
            \fw\HTTP::ContentType('css','utf-8');

            // Rendering typically starts at the Base template (see
            // fwboot.php and APP_ROOT/HTML/Base.tpl), with other
            // templates filling in different parts of the parent template.
            //
            // For CSS files, however, we want the full response to be CSS
            // so ReMap() the Base template to the CSS template.
            fw('html')->ReMap('Base',$F);
        }
        // The CSS file isn't known, so this is a 404.
        else
        {
            fw()->NoPageHandler();
        }

        // Execution now returns to fwboot.php and normal rendering will occur.
    }

    // Page Name: JS
    //  Page URL: /js/
    //
    // see CSSHandler()
    public static function JSHandler()
    {
        $F = 'js_'.str_replace('.js','',\fw\Path::Bottom(fw()->MatchPath));

        if( isset(fw('html')->{$F}) )
        {
            \fw\HTTP::ContentType('js','utf-8');
            fw('html')->ReMap('Base',$F);
        }
        else
        {
            fw()->NoPageHandler();
        }
    }

    // Page Name: Examples
    //  Page URL: /examples
    //
    // This Routine demonstrates typical web logic - conditional text display,
    // form submittal and populate, validation and database access.  It utilizes a
    // number of internal Routines (Internal.inc) and complimentary Ajax Routines
    // (Ajax.inc).
    //
    // Corresponding template HTML/examples/Home.tpl should be reviewed in
    // conjunction with this method.
    public static function Examples()
    {
        // Pull in our $page key/value store.
        $page = fw('page');

        // Pull in our database connections - we'll check later if any of them
        // are NULL (i.e., not connected).
        $mongodb = fw('mongodb');
        $mysql = fw('mysql');
        $sqlsrv = fw('sqlsrv');

        // Framewire's provides a number of built-in methods for working with
        // forms, including populating the list of possible values for form
        // selects.
        //
        // The keys/values of an associative array can be used as the values
        // and labels, respectively, for each option.
        //
        // For common lists - such as US states and name suffixes, Framewire's
        // US locale form handler provides a number of built-in sets:
        //   - \fw\enUS::StateListing()
        //   - \fw\enUS::PrefixListing()
        //   - \fw\enUS::SuffixListing()
        //
        // Here we define a custom list of options for the "How did you hear about us"
        // form field, Connect() it to $html so it's accessible in our templates,
        // and store in $page so it's available for use in Internal::MainForm().
        $HAU = array('Internet'=>'Internet','Mail'=>'Mailed brochure','Email'=>'Email blast',
                     'TV'=>'Television commercial','Radio'=>'Radio commercial',
                     'WoM'=>'Word of mouth');
        $page->HAU = $HAU;
        fw('html')->Connect($HAU,'HAU');


        // This page supports three basic operations:
        //   - Submit of a new record for creation
        //   - Load of an existing record for editing
        //   - Submit of an existing record for update
        //
        // It also sets different flags to describe the type of operation
        // and for which databases.  These flags are then used in templates
        // and other Routines.


        // First determine if a form was submitted and if so, which button.
        if( \fw\enUS::IsSubmit('Submit',$_POST,'SaveToMongo') )
            $Submitted = 'Mongo';
        else if( \fw\enUS::IsSubmit('Submit',$_POST,'SaveToMySQL') )
            $Submitted = 'MySQL';
        else if( \fw\enUS::IsSubmit('Submit',$_POST,'SaveToSQLSrv') )
            $Submitted = 'SQLSrv';
        else
            $Submitted = NULL;


        // Next, determine if we're doing an update.
        //
        // Here we alter our in-page URLs so that our ID is propagated,
        // and we set flags in $page for in-Template control of HTML,
        // depending on the database that's getting the update.
        if( !empty($_GET['mongoedit']) )
        {
            $page->MongoUpdate = $Update = 'Mongo';

            // Create a URL change string to propagate our ID
            // see LinkPage
            $page->FormChangeString = "?mongoedit={$_GET['mongoedit']}";
        }
        else if( !empty($_GET['mysqledit']) )
        {
            $page->MySQLUpdate = $Update = 'MySQL';
            $page->FormChangeString = "?mysqledit={$_GET['mysqledit']}";
        }
        else if( !empty($_GET['sqlsrvedit']) )
        {
            $page->SQLSrvUpdate = $Update = 'SQLSrv';
            $page->FormChangeString = "?sqlsrvedit={$_GET['sqlsrvedit']}";
        }
        else
            $page->Update = $Update = NULL;


        // If we're trying to submit to or update a particular database, ensure
        // we have a valid connection to it and that the database exists.
        if( ($Submitted === 'Mongo' || $Update === 'Mongo') && $page->MongoDBExists !== TRUE )
            throw new Exception('Mongo not connected.');

        if( ($Submitted === 'MySQL' || $Update === 'MySQL') && $page->MySQLDBExists !== TRUE )
            throw new Exception('MySQL not connected.');

        if( ($Submitted === 'SQLSrv' || $Update === 'SQLSrv') && $page->SQLSrvTablesExist !== TRUE )
            throw new Exception('SQLSrv not connected.');


        // Populate our form with data from the database.
        //
        // Framewire's CRUDC (CREATE/READ/UPDATE/DELETE/COUNT) interface is used
        // for all three databases.
        //
        // Here we set $_POST to the record from the database if an update is requested
        // but no form has been submitted.
        if( $Update !== NULL && $Submitted === NULL )
        {
            if( $Update === 'Mongo' )
            {
                // Use the Framewire CRUDC interface to quickly read a record.
                $R = $mongodb->READ('Profile',array('_id'=>new MongoID($_GET['mongoedit'])));
                if( empty($R) )
                {
                    // Use the Messanger object to set a one-time in-page message
                    // and set flags to indicate that we don't actually have an update.
                    // see HTML/examples/Home.tpl
                    fw('msg')->FormMsg = 'Mongo record not found.';

                    // see HTML/ajf/submit-buttons.tpl
                    $page->MongoUpdate = $page->Update = $page->FormChangeString = NULL;
                    return;
                }

                // Mongo returns an associative array of records, keyed by _id. Here we
                // fetch the first record directly into $_POST.
                $_POST = current($R);
            }

            if( $Update === 'MySQL' )
            {
                $R = $mysql->READ('Profile',array('ProfileID'=>$_GET['mysqledit']));
                if( empty($R[0]) )
                {
                    fw('msg')->FormMsg = 'MySQL record not found.';
                    $page->MySQLUpdate = $page->Update = $page->FormChangeString = NULL;
                    return;
                }

                // MySQL returns a numeric array of rows.
                $_POST = $R[0];
            }

            if( $Update === 'SQLSrv' )
            {
                $R = $sqlsrv->READ('Profile',array('ProfileID'=>$_GET['sqlsrvedit']));
                if( empty($R[0]) )
                {
                    fw('msg')->FormMsg = 'SQL Server record not found.';
                    $page->SQLSrvUpdate = $page->Update = $page->FormChangeString = NULL;
                    return;
                }

                // SQLSrv returns a numeric array of rows.
                $_POST = $R[0];
            }

            // Fine tune field names and structure, depending on the database in use.
            if( $Update !== 'Mongo' )
                $_POST['HAU'] = explode(',',$_POST['HAU']);

            $_POST['Desc'] = $_POST['Description'];
            $_POST['CCN'] = $_POST['CCNumber'];
            $_POST['Password2'] = $_POST['Password'];
        }


        // Validate the form if it's been submitted.
        if( $Submitted !== NULL )
        {
            // Here we use an additional Routine to perform the validation.
            // These routines are not tied to a Page/URL.

            // see the MainForm() method in Routines/Internal.inc for how to
            // validate a form using Framewire tools.
            Internal::MainForm();

            // The validation status has been stored in the $vr ValidationReport
            // by the processing performed in the above method.
            //
            // Here we use fw() to pull in the object, and then determine
            // whether the form is valid.
            $vr = fw('vr');

            // The form was not valid so set an appropriate message and return.
            // see HTML/examples/Home.tpl for using ValidationReport to highlight
            // form fields with errors and enUSHTMLSet for repopulating form data.
            if( !$vr() )
            {
                fw('msg')->FormMsg = 'Please check the form fields.';
                return;
            }
        }


        // At this stage, a form has been submitted and it's valid.
        //
        // Here we use the form data and Framewire's CRUDC interface to either
        // create or update records in the appropriate database.
        if( $Submitted === 'Mongo' )
        {
            // MongoDB::Map() associates the fields of an array, in this case
            // the form data in $_POST, to the columns of our database.  However,
            // since MongoDB collections don't have a fixed set of columns,
            // MongoDB::Map() uses the ValidationReport ($vr) to define which
            // fields should be stored.
            //
            // Mongo::Map() also supports overrides for renaming or ignoring fields.
            // For example, the form field CCN will be stored as CCNumber and
            // Desc will be stored as Description.  Field names not defined in
            // $vr will be ignored.
            $M = $mongodb->Map($vr,$_POST,array('CCN'=>'CCNumber','Desc'=>'Description'));
            $M['InsertDT'] = new MongoDate;

            // Perform an update using our mapped array, $M.
            //
            // Note that Mongo will report 1 affected row, even if no data has changed.
            if( $Update === $Submitted )
            {
                $RA = $mongodb->UPDATE('Profile',$M,array('_id'=>new MongoID($_GET['mongoedit'])));
                fw('msg')->FormMsg = "MongoDB: Updated {$RA} row.";
            }
            // Create a new record using our mapped array, $M.
            else
            {
                // Use MongoDB::CREATE() to insert a new record.  The automatically
                // generated MongoID _id is returned.
                $ProfileID = $mongodb->CREATE('Profile',$M);

                // Handle file uploads using Framewire's \fw\FileUpload toolset.
                //
                // Since MongoDB cannot dictate which fields a record should contain,
                // we set three fields as being valid for our data Map() call.
                $vr->Filename = $vr->FileSize = $vr->ContentType = TRUE;

                // FileUpload::Init() has been called already in MainForm() during
                // validation, however we can call it again here to quickly access
                // data about any file uploads.
                $FileIDs = array();
                foreach( \fw\FileUpload::Init('FileUp') as $F )
                {
                    // Map and set the file upload data fields that we want to store.
                    $M = $mongodb->Map($vr,$F);
                    $M['Profile_id'] = $ProfileID;
                    $M['FileData'] = new MongoBinData(file_get_contents($F['TmpPath']),MongoBinData::BYTE_ARRAY);
                    $FileIDs[] = $mongodb->CREATE('FileUpload',$M);
                }

                // Data storage is complete - set an appropriate user message.
                fw('msg')->FormMsg = 'Save to Mongo as ProfileID '.(string)$ProfileID;
                if( !empty($FileIDs) )
                {
                    fw('msg')->FormMsg = fw('msg')->FormMsg.' with FileIDs ';
                    foreach( $FileIDs as $F )
                        fw('msg')->FormMsg = fw('msg')->FormMsg." $F ";
                }
            }
        }

        // For MySQL, we can proceed similarly as we did for Mongo, with the
        // following caveats noted below.
        if( $Submitted === 'MySQL' )
        {
            // We'll use CRUDC methods to insert or update rows.  All values
            // are automatically escaped using mysqli::real_escape_string().

            // MySQL::Map() associates the fields of an array, in this case
            // the form data in $_POST, to the columns of our table by reading
            // table structure details from MySQL's information_schema.
            //
            // MySQL::Map() also supports overrides and field names that
            // aren't columns in the table, such as Submit, will be ignored.
            //
            // NOTE: The ordering (column name => array element) is reversed from
            // that of Mongo::Map().
            $M = $mysql->Map('Profile',$_POST,array('CCNumber'=>'CCN','Description'=>'Desc'));

            // MySQL's CRUDC interface understands some special notation.  For
            // example, preceeding a value with a pipe (|) will prevent it from
            // being escaped, useful for simple SQL functions.
            $M['InsertDT'] = '|NOW()';

            // MySQL doesn't support arrays.
            $M['HAU'] = implode(',',$M['HAU']);

            // Perform an update using our mapped array, $M.
            //
            // Note that MySQL will report 1 affected row if data has changed
            // or TRUE if no data has changed.  In this case, however, since
            // InsertDT is updated, this will always be 1.
            if( $Update === $Submitted )
            {
                $RA = $mysql->UPDATE('Profile',$M,array('ProfileID'=>$_GET['mysqledit']));
                fw('msg')->FormMsg = "MySQL: Updated {$RA} row.";
            }
            else
            {
                // Use MySQL::CREATE() to insert a new record.  The auto-incremented
                // primary key is returned.
                $ProfileID = $mysql->CREATE('Profile',$M);

                // Proceed to handle file uploads.
                $FileIDs = array();
                foreach( \fw\FileUpload::Init('FileUp') as $F )
                {
                    // Mapping an element key to NULL will force it to be ignored.
                    $M = $mysql->Map('FileUpload',$F,array('TmpPath'=>NULL));
                    $M['R_ProfileID'] = $ProfileID;
                    $M['FileData'] = file_get_contents($F['TmpPath']);

                    $FileIDs[] = $mysql->CREATE('FileUpload',$M);
                }

                fw('msg')->FormMsg = "Saved to MySQL as Profile ID $ProfileID";
                if( !empty($FileIDs) )
                {
                    fw('msg')->FormMsg = fw('msg')->FormMsg.' with FileIDs ';
                    foreach( $FileIDs as $F )
                        fw('msg')->FormMsg = fw('msg')->FormMsg." $F ";
                }
            }
        }

        // For SQL Server, we can proceed similarly as we did for MySQL, with the
        // following caveats noted below.
        if( $Submitted === 'SQLSrv' )
        {
            // We'll use CRUDC methods to insert or update rows.  SQL Server
            // uses prepared statements.

            $M = $sqlsrv->Map('Profile',$_POST,array('CCNumber'=>'CCN','Description'=>'Desc'));
            $M['InsertDT'] = '|GETDATE()';
            $M['HAU'] = implode(',',$_POST['HAU']);

            // Perform an update using our mapped array, $M.
            //
            // Note that SQL Server will show 1 affected row, even if no
            // data has changed.
            if( $Updated === $Submitted )
            {
                $RA = $sqlsrv->UPDATE('Profile',$M,array('ProfileID'=>$_GET['sqlsrvedit']));
                fw('msg')->FormMsg = "SQLSrv: Updated {$RA} row.";
            }
            else
            {
                $ProfileID = $sqlsrv->CREATE('Profile',$M);

                $FileIDs = array();
                foreach( \fw\FileUpload::Init('FileUp') as $F )
                {
                    $M = $sqlsrv->Map('FileUpload',$F,array('TmpPath'=>NULL));
                    $M['R_ProfileID'] = $ProfileID;
                    $M['FileData'] = file_get_contents($F['TmpPath']);

                    // The FileData column will be SQL typed binary stream because it's
                    // column type is varbinary.  This is automatically handled by
                    // SQLSrv's CRUDC interface.
                    $FileIDs[] = $sqlsrv->CREATE('FileUpload',$M);
                }

                fw('msg')->FormMsg = "Saved to SQL Server as Profile ID $ProfileID";
                if( !empty($FileIDs) )
                {
                    fw('msg')->FormMsg = fw('msg')->FormMsg.' with FileIDs ';
                    foreach( $FileIDs as $F )
                        fw('msg')->FormMsg = fw('msg')->FormMsg." $F ";
                }
            }
        }

        // Lastly, if the user has requested a copy by email, send it.
        //
        // The email address has already been validated, thus, if the field isn't
        // empty, it's an email address.
        if( $Submitted !== NULL && !empty($_POST['EmailACopy']) )
            \fw\Log::Email(\fw\Debug::Dump($_POST),'LOG',NULL,NULL,$_POST['EmailACopy']);
    }
}


