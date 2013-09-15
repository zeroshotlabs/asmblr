<?php
namespace asm;

/**
 * FileSet manages multiple ordered lists in a directory/file filesystem modal for one or more Site_ids.
 *
 * Two collections are used each containing their own types of records:
 *  Dir: includes name, type, current timestamp and optional parent and meta data.
 *  File: includes data, current timestamp and optional type.
 *
 * We use UNIX/Windows filesystem command naming convention - ls, rm, rmdir, mkdir - and treat this
 * as in a filesystem model that's Mongo backed.  However dir/file names are not database-side unique.
 *
 * For most methods that expect a directory parameter, the following forms are accepted:
 *  - a MongoId
 *  - a string, taken as the Name
 *  - an array constraint of either Name=>'String','Type'=>'String','Parent_id'=>MongoId
 *
 *  When a string or constraint array is used, an additional FileSet::stat() is performed.  When a name, type or Parent_id
 *  is used, it could resolve to more than one Dir in which case most methods will use the database's "first" one.
 *
 * @todo Is file content (like images) better here than everything in ContentSet?  ContentSet would
 *       be then just renderable content.
 * @todo Possibly support "symlinks" of a file between directories.
 */

/**
 * Directory Struct for Mongo.
 * Default Collection: FileSetDir
 */
abstract class Dir extends MongoStruct
{
    protected static $Skel = array('Site_id'=>NULL,'Name'=>NULL,'Type'=>NULL,'Parent_id'=>NULL,'CurrentTS'=>0,'Meta'=>array());


    /**
     * Initialize a Dir Mongo Struct.
     *
     * @param string $Name Required user-defined name (non-unique).
     * @param string $Type Required user-defined type (non-unique).
     * @param \MongoId $Parent_id Optional parent that may refer to another Dir or any MongoId.
     * @retval array Dir struct.
     *
     * @note Site_id is set by the ListSet object when saved.
     */
    public static function Init( $Name,$Type,\MongoId $Parent_id = NULL )
    {
        $L = static::$Skel;
        $L['Name'] = $Name;
        $L['Type'] = $Type;
        $L['Parent_id'] = $Parent_id;

        $L['CurrentTS'] = time();

        return $L;
    }

    public static function SetMeta( $Meta,&$L )
    {
        if( !is_array($Meta) )
            throw new Exception('Dir Meta must be array');

        $L['Meta'] = $Meta;
    }
}

/**
 * File Struct for Mongo.
 * Default Collection: FileSet
 *
 * @note Currently DirO is initialized as an int though it's type is not enforced and mixing the two types
 *       will probably lead to bad results (expand/contracting a non-string doesn't work).
 * @note Complex sorting should be app-side.
 */
abstract class File extends MongoStruct
{
    protected static $Skel = array('Site_id'=>NULL,'Dir_id'=>NULL,'DirO'=>0,'Type'=>'','CurrentTS'=>0,'Data'=>array());


    /**
     * Initialize a File Mongo Struct.
     *
     * @param string $Data Required user-defined data to store (non-unique).
     * @param string $Type Optional user-defined type.
     * @retval array Dir struct.
     *
     * @note Site_id, Dir_id and DirO is set by the ListSet object when saved.
     * @note A File doesn't actually have a name, currently.
     */
    public static function Init( $Data,$Type = '' )
    {
        $I = static::$Skel;
        $I['Data'] = $Data;

        $I['Type'] = $Type;
        $I['CurrentTS'] = time();

        return $I;
    }
}

class FileSet extends \fw\WireableBase
// NOTE: For iteration, a mongocursor is returned; this class doesn't do iteration
// implements \Iterator,\Countable
// ,\ArrayAccess
{
    protected $Site_id;
    protected $MDB;
    protected $MDCd;
    protected $MDC;


    public function __construct( \fw\MongoDB $MDB,\MongoId $Site_id,$Collection = NULL )
    {
        $this->Site_id = $Site_id;

        if( empty($Collection) )
            $Collection = str_replace('asm\\','',get_class($this));

        $this->MDB = $MDB;
        $this->MDCd = $MDB->{$Collection.'Dir'};
        $this->MDC = $MDB->$Collection;

        // temp :)
        // per Dir Struct
        $this->MDCd->ensureIndex(array('Site_id'=>TRUE,'Name'=>TRUE),array('unique'=>FALSE));
        $this->MDCd->ensureIndex(array('Site_id'=>TRUE,'Type'=>TRUE),array('unique'=>FALSE));
        $this->MDCd->ensureIndex(array('Site_id'=>TRUE,'Parent_id'=>TRUE),array('unique'=>FALSE));

        // per File Struct - we can't have a unique index because Mongo can't do an ordered update... ideas?
        $this->MDC->ensureIndex(array('Site_id'=>TRUE,'Dir_id'=>TRUE,'DirO'=>TRUE,'CurrentTS'=>TRUE),array('unique'=>FALSE));
        $this->MDC->ensureIndex(array('Site_id'=>TRUE,'Dir_id'=>TRUE,'Type'=>TRUE,'DirO'=>TRUE),array('unique'=>FALSE));
    }

    /**
     * Returns list of available lists as Dir structs.
     *
     * @param array $Constraint Optional database-side constraint to override default.
     * @param array $Sort Optional database-side sort.
     * @retval \MongoCursor
     *
     * @note To retrieve files, use ls()
     */
    public function dir( $Constraint = array(),$Sort = array() )
    {
        $C = array('Site_id'=>$this->Site_id);

        if( !empty($Constraint) )
            $C = array_merge($C,$Constraint);

        return $this->MDCd->find($C)->sort($Sort);
    }

    /**
     * Create a new list.
     *
     * @param string $Name User-defined list name.
     * @param string $Type User-defined list type.
     * @param \MongoId $Parent_id Optional parent _id (could be of a list or File)
     * @param array $Meta Optional used-defined meta data.
     * @retval array Created Dir Struct.
     *
     * @todo Should $Name be unique across a Site_id ?
     */
    public function mkdir( $Name,$Type,\MongoId $Parent_id = NULL,$Meta = array() )
    {
        $L = Dir::Init($Name,$Type,$Parent_id);
        $L['Site_id'] = $this->Site_id;

        if( !empty($Meta) )
            Dir::SetMeta($Meta,$L);

        $this->MDCd->insert($L);

        return $L;
    }

    /**
     * Make a new File in a Dir.
     *
     * Dir must already exist.
     *
     * @param array $Data The File's contents.
     * @param \MongoId $Dir_id The list to create the File in.
     * @param string $Type Optional data type.
     * @param \MongoId $Before_id Optional File_id to create File before, otherwise append.
     * @param boolean $Before_id If TRUE, the File will be prepended.
     * @retval array Newly created File Struct.
     *
     * @throws Exception Dir_id must exist for mk.
     * @todo To move, rm() then this mk() works, but for "symlink" items that'd be a problem since the _id could change as things are now
     * @note File ordering always starts at 1 - MaxDirO returns 0 and we always add one.
     */
    public function mk( $Data,\MongoId $Dir_id,$Type = '',$Before_id = NULL )
    {
        if( $this->MDCd->findOne(array('_id'=>$Dir_id)) === NULL )
            throw new Exception('Dir_id must exist for mk.');

        $I = File::Init($Data,$Type);
        $I['Site_id'] = $this->Site_id;
        $I['Dir_id'] = $Dir_id;

        $DirO = NULL;

        // prepend
        if( $Before_id === TRUE )
        {
            $F = $this->MDC->findOne(array('Site_id'=>$this->Site_id,'Dir_id'=>$Dir_id,'DirO'=>1),array('_id'=>TRUE));
            if( !empty($F) )
                $Before_id = $F['_id'];
        }

        // insert before this File _id (assumes belongs to our Dir_id)
        if( $Before_id instanceof \MongoId )
        {
            $DirO = $this->MDC->findOne(array('_id'=>$Before_id),array('DirO'=>TRUE));
            if( !empty($DirO) )
            {
                $this->Expand($DirO['DirO'],$Dir_id);
                $DirO = $DirO['DirO'];
            }
        }

        // before_id not supplied or doesn't exist so append
        if( $Before_id === NULL || $DirO === NULL )
            $DirO = $this->MaxDirO($Dir_id) + 1;

        $I['DirO'] = $DirO;

        $this->MDC->insert($I);
        return $I;
    }

    /**
     * Resolve a dir or file to it's struct.
     *
     * The following forms can be resolved:
     *  - a MongoId, taken as the Dir or File _id
     *  - a string, taken as a Dir Name (File lookup is not attempted)
     *  - a constraint array of either Name=>'String','Type'=>'String','Parent_id'=>MongoId (File lookup is not attempted)
     *
     * @param \MongoId $Dir Dir or File _id to check.
     * @param string $Dir Dir Name to check.
     * @param array $_id Constraint array to check.
     * @retval array The Dir or File Struct.
     * @retval NULL The _id could not be resolved.
     */
    public function stat( $_id,$Full = FALSE )
    {
        if( $_id instanceof \MongoId )
        {
            $R = $this->MDCd->findOne(array('_id'=>$_id));
            if( !empty($R) )
                return $R;

            $R = $this->MDC->findOne(array('_id'=>$_id));
            if( !empty($R) )
                return $R;

            return NULL;
        }
        else if( is_string($_id) )
        {
            return $this->MDCd->findOne(array('Site_id'=>$this->Site_id,'Name'=>$_id));
        }
        else if( is_array($_id) )
        {
            return $this->MDCd->findOne(array('Site_id'=>$this->Site_id)+$_id);
        }
        else
            return NULL;
    }

    /**
     * Count how many Files in a Dir.
     *
     * @param mixed $D The dir to count files of (stat() compatible).
     * @throws Exception Invalid Dir Struct.
     */
    public function count( $D )
    {
        if( $D instanceof \MongoId )
            return $this->MDC->count(array('Site_id'=>$this->Site_id,'Dir_id'=>$D));
        else
        {
            $D = $this->stat($D);
            if( !empty($D) )
                return $this->MDC->count(array('Site_id'=>$this->Site_id,'Dir_id'=>$D['_id']));
            else
                return 0;
        }
    }

    /**
     * Return directory listing as File structs.
     *
     * @param mixed $D The dir to list files of (stat() compatible).
     * @param array $Constraint Optional database-side constraint to override default.
     * @param array $Sort Optional database-side sort or DirO ascending by default.
     * @retval \MongoCursor
     * @retval array Empty array if the directory could not be resolved.
     * @throws Exception Invalid Dir parameter.
     * @throws No directory Name '$Dir'
     *
     * @note When listing by name, if more than one directory has the same name, only
     *       the "first" will be listed.
     */
    public function __invoke( $D,$Constraint = array(),$Sort = array() )
    {
        if( !($D instanceof \MongoId) )
        {
            $D = $this->stat($D);
            if( empty($D) )
                return array();
            $D = $D['_id'];
        }

        $C = array('Site_id'=>$this->Site_id,'Dir_id'=>$D);

        if( !empty($Constraint) )
            $C = array_merge($C,$Constraint);

        $S = array('DirO'=>1);

        if( !empty($Sort) )
            $S = array_merge($S,$Sort);

        return $this->MDC->find($C)->sort($S);
    }

    /**
     * Alias of __invoke() for listing contents.
     */
    public function ls( $D,$Constraint = array(),$Sort = array() )
    {
        return $this->__invoke($D,$Constraint,$Sort);
    }

    /**
     * Delete a Dir and all of it's Files.
     *
     * @param mixed $D The dir to delete (stat() compatible).
     * @retval boolean TRUE if the directory was resolved and deleted.
     * @throws Exception Invalid Dir Struct.
     */
    public function rmdir( $D )
    {
        $D = $this->stat($D);
        if( empty($D) )
            return FALSE;

        $this->MDCd->remove(array('_id'=>$D['_id']));
        $this->MDC->remove(array('Dir_id'=>$D['_id']),array('justOne'=>FALSE));
    }

    /**
     * Delete an File.
     *
     * @param array $I The File Struct to delete.
     * @throws Exception Invalid File Struct.
     *
     * @note The File won't be deleted unless CurrentTS matches.
     */
    public function rm( $I )
    {
        if( empty($I['_id']) || !($I['_id'] instanceof \MongoId) )
            throw new Exception('Invalid File Struct');

        $R = $this->MDC->remove(array('_id'=>$I['_id'],'CurrentTS'=>$I['CurrentTS']));
        if( $R['n'] === 1 )
            $this->Contract($I['DirO'],$I['Dir_id']);
    }

    /**
     * Set the Data contents of a File.
     *
     * @param array $Data Array of data to set.
     * @param array $I Existing File struct.
     *
     * @note The File won't be updated unless CurrentTS matches.
     */
    public function set( $Data,&$I )
    {
        $I['Data'] = $Data;

        $R = $this->MDC->update(array('_id'=>$I['_id'],'CurrentTS'=>$I['CurrentTS']),
                                array('$set'=>array('Data'=>$Data)));

        return $R['n'];
    }

    /**
     * Set a particular Data key/value for an File.
     *
     * The Key will be added if it doesn't exist.  If the File's contents aren't an array already,
     * i.e. a string, the results are undefined.
     *
     * @param string $Key The Data.Key to set.
     * @param mixed $Value The value to set.
     * @param array $I Existing File struct.
     *
     * @note The File won't be updated unless CurrentTS matches.
     */
    public function touch( $Key,$Value,&$I )
    {
        $I['Data'][$Key] = $Value;

        $R = $this->MDC->update(array('_id'=>$I['_id'],'CurrentTS'=>$I['CurrentTS']),
                                array('$set'=>array("Data.{$Key}"=>$Value)));

        return $R['n'];
    }

    /**
     * Return the current max value of DirO for a given Dir_id.
     *
     * 0 1 2 3 4 MaxDirO() = 4
     *
     * @param \MongoId $Dir_id The list to check.
     * @retval int The current maximum, or 0.
     *
     * @note This is noSQL so who knows if it's truely the max and Dir_id is assumed to exist.
     */
    protected function MaxDirO( \MongoId $Dir_id )
    {
        $O = $this->MDC->find(array('Site_id'=>$this->Site_id,'Dir_id'=>$Dir_id),array('DirO'=>TRUE))->sort(array('DirO'=>-1))->limit(1)->getNext();
        if( empty($O) )
            return 0;
        else
            return $O['DirO'];
    }

    /**
     * Expand a list by 1 starting at and including DirO  (creates a gap before position specified).
     *
     * 0 1 2 3 4 + Expand(2) becomes 0 1 _ 3 4 5
     *
     * @param int $DirO DirO to start expansansion at.
     * @param \MonogoId $Dir_id The list to expand.
     * @retval int The number of Files affected which could be 0.
     *
     * @note As far as we can see, Mongo doesn't support ordered update, so we can't have a unique index.
     *  maybe some day:  '$orderby'=>array('Dir_id'=>-1)
     */
    protected function Expand( $DirO,\MongoId $Dir_id )
    {
        $R = $this->MDC->update(array('Site_id'=>$this->Site_id,'Dir_id'=>$Dir_id,'DirO'=>array('$gte'=>$DirO)),
                                array('$inc'=>array('DirO'=>1)),array('multiple'=>TRUE));
        return $R['n'];
    }

    /**
     * Contract a list by 1 starting at and including DirO  (closes a gap at position specified).
     *
     * 0 1 _ 3 4 5 + Contract(2) becomes 0 1 2 3 4
     *
     * @param int $DirO DirO to start contraction at (which shouldn't actually exist).
     * @param \MonogoId $Dir_id The list to contract.
     * @retval int The number of Files affected which could be 0.
     *
     * @note The gap must already exist.
     *
     * '$orderby'=>array('Dir_id'=>-1)
     */
    protected function Contract( $DirO,\MongoId $Dir_id )
    {
        $R = $this->MDC->update(array('Site_id'=>$this->Site_id,'Dir_id'=>$Dir_id,'DirO'=>array('$gt'=>$DirO)),
                                array('$inc'=>array('DirO'=>-1)),array('multiple'=>TRUE));
        return $R['n'];
    }

    public function ApplyDirective( $Key,$Value )
    {
        throw new \fw\Exception("Not supported in FileSet {$this->DictTag}");
    }
}

