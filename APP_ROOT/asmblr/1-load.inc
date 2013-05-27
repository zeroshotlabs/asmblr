<?php

namespace {
function asm( $Name = NULL )
{
    if( is_string($Name) === TRUE )
        return $GLOBALS['ASMAPP']->$Name;
    else if( is_array($Name) === TRUE )
        return $GLOBALS['ASMAPP']->GetWired($Name);
    else if( $Name === NULL )
        return $GLOBALS['ASMAPP'];
    else
        return $GLOBALS['ASMAPP']->__isset($Name);
}
}

namespace asm
{
// tmp - might need more
class LinkPage extends \fw\LinkPage
{
	public function __construct( \asm\PageSet $PageSet,$BaseURL = NULL,$Set = array() )
	{
		$this->PageSet = $PageSet;
		$this->SetBaseURL($BaseURL,$Set);
	}
	// we need PageSet::ReadByName() and some other stuff for this to work
// 	public function __invoke( $Name = NULL,$Set = array() )
// 	{
		

// 	}
	
}

	
abstract class JSONResponse extends \fw\Struct
{
    protected static $Skel = array('Method'=>'','Status'=>TRUE,'Msg'=>'','Data'=>'');

    public static function Ok( $Data )
    {
        $R = static::$Skel;
        $R['Method'] = \fw\Path::ToURLString(fw()->Request['Path']);
        $R['Data'] = $Data;
        return $R;
    }

    public static function Fail( $Msg )
    {
        $R = static::$Skel;
        $R['Method'] = \fw\Path::ToURLString(fw()->Request['Path']);
        $R['Status'] = FALSE;
        $R['Msg'] = $Msg;
        return $R;
    }
}

abstract class MongoStruct extends \fw\Struct
{
    // retrieve the created TS which is part of _id's MongoId
    public static function TS( $M )
    {
        if( isset($M['_id']) && $M['_id'] instanceof \MongoId )
            return $M['_id']->getTimestamp();
        else
            return NULL;
    }
}


// generic mongo setter - base for storing stuff in mongo via the REST API
class MongoSet extends \fw\WireableBase
{
    protected $MDB;
    protected $MDC;


    public function __construct( \fw\MongoDB $MDB,$Collection = NULL )
    {
        $this->MDB = $MDB;
        if( empty($Collection) )
            $Collection = str_replace('asm\\','',get_class($this));

        $this->MDC = $MDB->$Collection;
    }

    // CurrentTS isn't doing much of anything right now
    // it'd have to be provided by the request - maybe we don't even need it,
    // except for big settings like directives?
    protected function SetKV( $Key,$Value,$PK,&$H )
    {
        $R = $this->MDC->update(array($PK=>$H[$PK],'CurrentTS'=>$H['CurrentTS']),
                                array('$set'=>array($Key=>$Value)),
                                array('multiple'=>FALSE));

        if( empty($R['err']) && $R['n'] === 1 )
        {
            $H[$Key] = $Value;
            return TRUE;
        }
        else
            return FALSE;
    }

    public function IsDupeE( \Exception $E,$Key = NULL )
    {
        $MDB = $this->MDB;
        return $MDB::IsDupeE($E,$Key);
    }
}


class SessionStoreMongoDB implements \SessionHandlerInterface
{
    protected $MDC;

    public function __construct( \fw\MongoDB $MDB,$Collection = 'Session' )
    {
        $this->MDC = $MDB->$Collection;
    }
    public function open( $save_path,$session_id )
    {
        return TRUE;
    }
    public function close()
    {
        return TRUE;
    }
    public function read( $session_id )
    {
        $R = $this->MDC->findOne(array('_id'=>$session_id));

        if( !empty($R) )
            return $R['d'];
        else
            return '';
    }
    public function write( $session_id,$session_data )
    {
        $this->MDC->update(array('_id'=>$session_id),array('_id'=>$session_id,'cts'=>new \MongoDate,'d'=>$session_data),array('upsert'=>TRUE));
        return TRUE;
    }
    public function destroy( $session_id )
    {
        // should we not remove it and let GC handle it?
        $this->MDC->remove(array('_id'=>$session_id));
        return TRUE;
    }
    // maybe not correct - can we avoid having cts?  if not, need an index on it
    public function gc( $maxlifetime )
    {
        $this->MDC->remove(array('$lt'=>array('cts'=>new \MongoDate(time()-$maxlifetime))));
        return TRUE;
    }
}

}

