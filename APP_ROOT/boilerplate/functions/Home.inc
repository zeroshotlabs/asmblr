<?php

abstract class Stuff
{
    public static function Home( $app )
    {
        $mdb = $app->mydb;

        // only CRUDC methods provide debugging
        $mdb->CREATE('test',array('ts'=>date('r')));

        foreach( $mdb->READ('test') as $V )
            var_dump($V);
    }
}

