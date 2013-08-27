<?php
namespace asm;


// Collection: ContentSet
// would we want something like Site_id/Tag/Name as unique fields?
// content is active by default
abstract class Content extends MongoStruct
{
    protected static $Skel = array('Site_id'=>'','Path'=>'','CurrentTS'=>0,
                                   'Status'=>'Active','PathStruct'=>array(),
                                   'Type'=>'','Length'=>0,
                                   'Body'=>'','Meta'=>array());

    // Path, Body, Type (or detected)
    // stored as UTF8 text or MongoBin based on type
    public static function Init( \MongoId $S,$Path,$Body,$Type = '' )
    {
        $Content = static::$Skel;
        $Content['Site_id'] = $S;

        static::SetPath($Path,$Content);

//         $T = \fw\Path::Init(strtolower($Path));
//         $T['IsDir'] = $T['IsAbs'] = FALSE;

//         $Content['Path'] = \fw\Path::ToURLString($T);
//         $Content['PathStruct'] = $T;

        static::SetBody($Body,$Content,$Type);
        return $Content;
    }

    // normalize a path for how content likes it - return as a string
    public static function Path2Path( $Path )
    {
        // content can't live at root
        if( $Path['IsRoot'] === TRUE )
            return NULL;

        $Path['IsDir'] = $Path['IsAbs'] = FALSE;

        \fw\Path::Lower($Path);

        return \fw\Path::ToURLString($Path);
    }


    public static function SetPath( $Path,&$C )
    {
        $Path = \fw\Path::Init(strtolower($Path));
        $Path['IsDir'] = $Path['IsAbs'] = FALSE;

        $C['PathStruct'] = $Path;
        $C['Path'] = \fw\Path::ToString($Path);
    }

    // type is automatically set when setting Body, though this can set it explicitly afterwards
    public static function SetType( $Type,&$C )
    {
        $C['Type'] = \fw\HTTP::ResolveContentType($Type);
    }

    // modifies Type, Body and Length - an empty body is always text/plain
    public static function SetBody( $Body,&$C,$Type = '' )
    {
        if( empty($Body) )
        {
            $C['Body'] = '';

            if( empty($Type) )
                $C['Type'] = \fw\HTTP::ResolveContentType('txt');
            else
                $C['Type'] = \fw\HTTP::ResolveContentType($Type);

            $C['Length'] = 0;
        }
        else
        {
            if( empty($Type) )
            {
                $finfo = new \finfo(FILEINFO_MIME);
                $C['Type'] = $finfo->buffer($Body);

                // if a text type, try to use the filename since finfo can be unreliable for text
                // otherwise we'll stick with that finfo gave us
                // this still doesn't work 100% for small snippets of HTML/CSS/PHP
                if( strpos($C['Type'],'text/') === 0 )
                {
                    $T = \fw\HTTP::Filename2ContentType(\fw\Path::Bottom($C['PathStruct']));
                    if( !empty($T) )
                        $C['Type'] = $T;
                }
            }
            else
                $C['Type'] = \fw\HTTP::ResolveContentType($Type);

            // little hack for PHP
            if( strpos($C['Type'],'text/') === 0 || strpos($C['Type'],'php') !== FALSE )
            {
                \fw\Struct::ToUTF8($Body);
                $C['Body'] = $Body;
                $C['Length'] = strlen($C['Body']);
            }
            else
            {
                $C['Body'] = new \MongoBinData($Body,2);
                $C['Length'] = strlen($Body);
            }
        }
    }
}


// similar to PageSet
class ContentSet extends MongoSet
{
    protected $Site_id;


    // supports standalone mode like PageSet and TemplateSet
    public function __construct( \fw\MongoDB $MDB,\MongoId $Site_id = NULL,$Collection = NULL )
    {
        parent::__construct($MDB,$Collection);

        if( !empty($Site_id) )
            $this->Site_id = $Site_id;
    }

    // allowing _id is different from the others but may be handy
    public function Create( &$C,\MongoId $_id = NULL )
    {
        $C['_id'] = empty($_id)?new \MongoId:$_id;
        $this->MDC->insert($C);
    }

    // ContentSet is basically always in standalone mode but we stick with the naming convention
    public function Read_id( \MongoId $C )
    {
        return $this->MDC->findOne(array('_id'=>$C));
    }

    public function Delete( $C )
    {
        return $this->MDC->remove(array('_id'=>$C['_id'],'CurrentTS'=>$C['CurrentTS']))['n'];
    }

    public function SetStatus( $Status,&$C )
    {
        return $this->SetKV('Status',$Status,'_id',$C);
    }

    // could use some more validation or something maybe
    public function SetPath( $Path,&$C )
    {
        Content::SetPath($Path,$C);
        return $this->SetKV('Path',$C['Path'],'_id',$C) && $this->SetKV('PathStruct',$C['PathStruct'],'_id',$C);
    }

    public function SetBody( $Body,&$C )
    {
        Content::SetBody($Body,$C);

        // yay noSQL!
        $this->SetKV('Type',$C['Type'],'_id',$C);
        $this->SetKV('Body',$C['Body'],'_id',$C);

        return $this->SetKV('Length',$C['Length'],'_id',$C);
    }

    public function SetType( $Type,&$C )
    {
        Content::SetType($Type,$C);
        return $this->SetKV('Type',$C['Type'],'_id',$C);
    }

    // "atomic" :)  overwrites existing Meta data
    // not really used yet - probably should be converted to set particular field of meta (implemented in CB ADMIN API)
    public function SetMeta( $Meta,&$C )
    {
        return $this->SetKV('Meta',$Meta,'_id',$C);
    }

    // TODO: debugging
    // need to consider delivery directly out of mongo to the browser
    // this will handle with a path struct or assumed proper string
    public function Match( \MongoId $S,$Path )
    {
        // ditto here for Path - and really this practice probably makes sense for Page/PageSet too
//        $T = \fw\Path::Init($Path);

        if( is_array($Path) )
        {
            $Path['IsDir'] = $Path['IsAbs'] = FALSE;
            $Path = \fw\Path::ToURLString($Path);
        }

        if( ($C = $this->MDC->findOne(array('Site_id'=>$S,'Path'=>$Path))) !== NULL )
            return $C;
        else
            return NULL;
    }

    // will need to be handled differently somehow for Content
    // right now we're not pulling back the Body
    // iterator_to_array() is slow too
    public function Listing( \MongoId $S,$ASC = FALSE,$IncBody = FALSE )
    {
        if( $IncBody )
            return $this->MDC->find(array('Site_id'=>$S))->sort(array('_id'=>$ASC?1:-1));
        else
            return $this->MDC->find(array('Site_id'=>$S),array('Body'=>FALSE))->sort(array('_id'=>$ASC?1:-1));
    }
}


