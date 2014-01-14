<?php


// Connect to a SQL Server using provided credentials.
function ConnectSQLSrv( $I )
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

