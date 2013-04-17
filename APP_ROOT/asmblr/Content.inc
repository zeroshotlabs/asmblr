<?php
namespace asm;


// Collection: ContentSet
// Unique: DomainPath
abstract class Content extends MongoStruct
{
    protected static $Skel = array('DomainPath'=>'',
                                   'CurrentTS'=>0,'Status'=>'Unpublished',
                                   'Name'=>'','Body'=>'','Config'=>array());

    public static function Init( $Domain,$Path,$Body = '' )
    {
        $Content = static::$Skel;
        $T = \fw\Path::Init(strtolower('/'.$Path));
        $Content['DomainPath'] = strtolower($Domain).\fw\Path::ToURLString($T);
        $Content['Body'] = $Body;
        return $Content;
    }
}




