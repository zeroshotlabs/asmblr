<?php

// Connect to a MySQL server using provided credentials.
function ConnectMySQL( $I )
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
