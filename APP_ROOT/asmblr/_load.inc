<?php
namespace asm;


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



// generic mongo setter - base for storing in mongo
class MongoSet extends \fw\WireableBase
{
    protected $MDB;
    protected $MDC;


    public function __construct( \fw\MongoDB $MDB,$Collection = NULL )
    {
        $this->MDB = $MDB;
        if( empty($Collection) )
            $Collection = __CLASS__;

        $this->MDC = $MDB->$Collection;
    }

    protected function SetKV( $Key,$Value,$PK,&$H )
    {
        $R = $this->MDC->update(((array)$PK)+array('CurrentTS'=>$H['CurrentTS']),
                                array('$set'=>array($Key=>$Value)));

        if( empty($R['err']) )
        {
            $H[$K] = $Value;
            return TRUE;
        }
        else
            return FALSE;
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
        $this->MDC->update(array('_id'=>$session_id),array('_id'=>$session_id,'cts'=>new \MongoDate,'d'=>$session_data));
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


