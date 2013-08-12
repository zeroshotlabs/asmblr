<?php
namespace asm;

/*
//     protected static $Skel = array('OwningAccount_id'=>'',
//                                   'Users_id'=>array(array('_id'=>'','Level'=>'100')));

    public function Grant( $Email,$Domain,$Level )
    {
        $R = $this->MDB->SiteMeta->update(array('Domain'=>$Domain),array('$addToSet'=>array('Users'=>array('Email'=>$Email,'Level'=>(int)$Level))),array('safe'=>TRUE));
        return $R['n']===1?TRUE:FALSE;
    }

    public function Revoke( $Email,$Domain )
    {
        $this->MDB->SiteMeta->update(array('Domain'=>$Domain),array('$pull'=>array('Users'=>array('Email'=>$Email))));
    }
*/

// Collection: SiteSet
// Unique: Domain
// Directives treated the same way as Page/PageSet
abstract class Site extends MongoStruct
{
    protected static $Skel = array('Domain'=>'','AAID'=>'','CurrentTS'=>0,
                                   'Status'=>'Inactive','BaseURL'=>'',
                                   'Routine'=>array(),'Directives'=>array(),'Config'=>array());

    // do we set BaseURL the same as domain?
    public static function Init( $Domain,\MongoId $AAID )
    {
        $Site = static::$Skel;
        $Site['AAID'] = $AAID;
        static::SetDomain($Domain,$Site);

        return $Site;
    }

    public static function SetDomain( $Domain,&$S )
    {
        $S['Domain'] = strtolower($Domain);
    }

    public static function SetBaseURL( $BaseURL,&$S )
    {
        $S['BaseURL'] = \fw\URL::ToString(\fw\URL::Init($BaseURL));
    }

    // Routine is assumed a string and set as a struct
    public static function SetRoutine( $Routine,&$S )
    {
        if( trim($Routine) === '' )
            $S['Routine'] = array();
        else
            $S['Routine'] = \fw\Routine::Init($Routine);
    }

    // ditto per Page - domain is not saved
    public static function ToPHP( $S )
    {
        $Buf = "<?php\n\n";
        $Buf .= '$Status = '.var_export($S['Status'],TRUE).";\n";
        $Buf .= '$BaseURL = '.var_export($S['BaseURL'],TRUE).";\n\n";

        foreach( $S['Directives'] as $D )
            $Buf .= "\n\$Directives[] = ".var_export($D,TRUE).';';

        // review compatibility with Pointer routines
        $Buf .= "\n\n\n/*** START ROUTINE ***/";
        $Buf .= "\n\$Routine = <<<'_EO_ROUTINE_'\n".\fw\Struct::Get(0,$S['Routine'])."\n\n_EO_ROUTINE_;\n\n";

        return $Buf;
    }

    // see also Page::FromPHP
    public static function FromPHP( $PHP,&$ToSite )
    {
        $Directives = array();
        eval('?>'.$PHP);

        $ToSite = static::Init($ToSite['Domain'],$ToSite['AAID']);

        $ToSite['Status'] = $Status;
        static::SetBaseURL($BaseURL,$ToSite);
        $ToSite['Directives'] = $Directives;
        static::SetRoutine($Routine,$ToSite);
    }
}


// this is different from PageSet as it's only for management + "CRUD"
// of sites... runtime (Match/Execute) happens in asmSite
// ensureIndex(array('Domain'=>1),array('unique'=>TRUE));
class SiteSet extends MongoSet
{
    // Directives need to be saved separately, same as PageSet
    public function Create( &$S )
    {
        $S['_id'] = new \MongoId;
        $S['Directives'] = array();
        $this->MDC->insert($S);
    }

    public function Read( \MongoId $S )
    {
        $S2 = $this->MDC->findOne(array('_id'=>$S));
        if( !empty($S2) )
            $this->Flatten($S2);
        return $S2;
    }

    public function Delete( $S )
    {
        $R = $this->MDC->remove(array('_id'=>$S['_id'],'CurrentTS'=>$S['CurrentTS']))['n'];

        if( !empty($R) )
        {
            $ds = new DirectiveSet($this->MDB,$S['_id']);
            $ds->SiteDelete($S);
        }

        return $R;
    }

    public function SetDomain( $Domain,&$S )
    {
        Site::SetDomain($Domain,$S);
        return $this->SetKV('Domain',$Domain,'_id',$S);
    }

    public function SetStatus( $Status,&$S )
    {
        return $this->SetKV('Status',$Status,'_id',$S);
    }

    // shouldn't we be saving BaseURL as a Path Struct?
    public function SetBaseURL( $BaseURL,&$S )
    {
        Site::SetBaseURL($BaseURL,$S);
        return $this->SetKV('BaseURL',$BaseURL,'_id',$S);
    }

    public function SetRoutine( $Routine,&$S )
    {
        Site::SetRoutine($Routine,$S);
        return $this->SetKV('Routine',$S['Routine'],'_id',$S);
    }

    public function SetDirective( $Directive,&$S,\MongoId $D_id = NULL )
    {
        $ds = new DirectiveSet($this->MDB,$S['_id']);

        // create new directive, append to end
        if( empty($D_id) )
        {
            $ds->SiteSet($S,$Directive);
            return TRUE;
        }
        // update existing directive
        else
        {
            $D = $ds->Read($D_id);
            if( empty($D) )
                return FALSE;

            $ds->SetData($Directive,$D);
            return TRUE;
        }
    }

    public function DeleteDirective( \MongoId $D_id,&$S )
    {
        $ds = new DirectiveSet($this->MDB,$S['_id']);

        // good example of when we institute CurrentTS checking, we couldn't do this
        // as the Data struct would need to be passed to us
        $D = $ds->Read($D_id);
        if( empty($D) )
            return FALSE;
        else
        {
            $ds->Delete($D);
            return TRUE;
        }
    }

    // different - returns NULL or the new Data struct
    public function CopyDirective( \MongoId $D_id,&$S )
    {
        // why do we specify 'Directive' here?
        $ds = new DirectiveSet($this->MDB,$S['_id'],'Directive');

        $D = $ds->Read($D_id);
        if( empty($D) )
        {
            return NULL;
        }
        else
        {
            $NewO = $ds->OOf($D['_id']);
            return $ds->Move($D,$NewO+1,TRUE);
        }
    }

    // different - returns NULL or the new Data struct
    public function MoveDirective( \MongoId $D_id,\MongoId $NextD_id = NULL,&$S )
    {
        $ds = new DirectiveSet($this->MDB,$S['_id']);

        $D = $ds->Read($D_id);
        if( empty($D) )
            return NULL;

        // move to end
        if( empty($NextD_id) )
        {
            return $ds->Move($D,NULL);
        }
        else
        {
            $O = $ds->OOf($NextD_id);
            return $ds->Move($D,$O);
        }
    }

    public function SetConfig( $Config,&$S )
    {
        return $this->SetKV('Config',$Config,'_id',$S);
    }

    // each result isn't Flatten()'d
    public function Listing( \MongoId $AAID )
    {
        // $Fields = array('Path'=>1,'Domain'=>1,'Name'=>1,'Status'=>1,'CreateTS'=>1,'UpdateTS'=>1);
        return iterator_to_array($this->MDC->find(array('AAID'=>$AAID)));
    }

    // pull in directives into Directives element
    // when $S is then sent back via JSON, however, Directives gets munged up
    public function Flatten( &$S )
    {
        $ds = new DirectiveSet($this->MDB,$S['_id']);
        $ds->SiteTag($S);

        $S['Directives'] = $ds->SiteList($S);
    }

// loads an entire site (event content) from a zip file into memory (array)
// a dummy site id is used for templates/paegs/content (it's actually the account_id
    public static function SiteZip2Arr( $ZipFilePath,$Domain,\MongoId $Account_id )
    {
        $Z = new \ZipArchive;
        if( $Z->open($ZipFilePath) !== TRUE )
            throw new Exception("Failed to read zip file: $ZipFilePath");

        $Site = $Pages = $Templates = $Content = array();
        for( $i = 0; $i < $Z->numFiles; ++$i )
        {
            $Filename = $Z->statIndex($i)['name'];

            if( $Filename === 'Site.inc' )
            {
                $Site = Site::Init($Domain,$Account_id);
                Site::FromPHP($Z->getFromIndex($i),$Site);
            }
            else
            {
                $T = \fw\Path::Init($Filename);
                if( \fw\Path::Top($T) === 'Pages' && $Filename !== 'Pages/' )
                {
                    $Page = Page::Init($Account_id,str_replace('.inc','',\fw\Path::Bottom($T)),'/'.microtime());
                    Page::FromPHP($Z->getFromName($Filename),$Page);
                    $Pages[] = $Page;
                }
                else if( \fw\Path::Top($T) === 'Templates' && $Filename !== 'Templates/' )
                {
                    $Template = Template::Init($Account_id,str_replace('.inc','',\fw\Path::Bottom($T)));
                    Template::FromPHP($Z->getFromName($Filename),$Template);
                    $Templates[] = $Template;
                }
                else if( \fw\Path::Top($T) === 'Content' && $Filename !== 'Content/' )
                {
                    $Con = Content::Init($Account_id,str_replace('Content/','',$Filename),$Z->getFromIndex($i));
                    $Content[] = $Con;
                }
            }
        }

        return array('Site'=>$Site,'Pages'=>$Pages,'Templates'=>$Templates,'Content'=>$Content);
    }

    // ignores all files/directories except known ones and ending in .inc (everything in Content/)
    // dir can be anything and is not checked for security
    public static function SiteDir2Arr( $Dir,$Domain,\MongoId $Account_id )
    {
        $DirPath = realpath($Dir);
        if( empty($DirPath) )
            throw new Exception("Dir '$Dir' is not readable.");

        if( !is_readable("{$DirPath}/Site.inc") )
            throw new Exception("Dir '$DirPath' doesn't contain Site.inc.");

        $Site = Site::Init($Domain,$Account_id);
        Site::FromPHP(file_get_contents("{$DirPath}/Site.inc"),$Site);

        $Pages = array();
        if( ($LS = @scandir("{$DirPath}/Pages")) !== FALSE )
        {
            foreach( $LS as $File )
            {
                if( $File{0} === '.' || strpos($File,'.inc') === FALSE )
                    continue;

                $FilePath = "{$DirPath}/Pages".DIRECTORY_SEPARATOR.$File;

                $Page = Page::Init($Account_id,str_replace('.inc','',$File),'/'.microtime());
                Page::FromPHP(file_get_contents($FilePath),$Page);
                $Pages[] = $Page;
            }
        }

        $Templates = array();
        if( ($LS = @scandir("{$DirPath}/Templates")) !== FALSE )
        {
            foreach( $LS as $File )
            {
                if( $File{0} === '.' || strpos($File,'.inc') === FALSE )
                    continue;

                $FilePath = "{$DirPath}/Templates".DIRECTORY_SEPARATOR.$File;

                $Template = Template::Init($Account_id,str_replace('.inc','',$File));
                Template::FromPHP(file_get_contents($FilePath),$Template);
                $Templates[] = $Template;
            }
        }

        $Content = array();
        // need to recursive for sub-dirs?
        if( ($LS = @scandir("{$DirPath}/Content")) !== FALSE )
        {
            foreach( $LS as $File )
            {
                if( $File{0} === '.' )
                    continue;

                $FilePath = "{$DirPath}/Content".DIRECTORY_SEPARATOR.$File;

                $Con = Content::Init($Account_id,$File,file_get_contents($FilePath));
                $Content[] = $Con;
            }
        }

        return array('Site'=>$Site,'Pages'=>$Pages,'Templates'=>$Templates,'Content'=>$Content);
    }

    // try to save the site, and then ass pages, templates, content
    // assumes site array from SiteZip2Arr/etc - $Site is modified
    // if $Site['Site']['_id'] is empty, a new site is created, otherwise that _id is used and checked to exist
    // an existing page/template/content is deleted
    public static function SiteArr2Mongo( &$Site,$ss,$ps,$ts,$cs )
    {
        try
        {
            $S_id = $Site['Site']['Directives'];
            if( empty($Site['Site']['_id']) )
            {
                $ss->Create($Site['Site']);
            }
            else
            {
                $Site['Site'] = $ss->Read($Site['Site']['_id']);
            }

            foreach( $S_id as $D )
                $ss->SetDirective($D,$Site['Site']);
        }
        catch( \Exception $E )
        {
            if( $ss->IsDupeE($E) )
                throw new Exception('Duplicate Domain');
            else
                throw new Exception('Database error');

            return;
        }

        $ps->UseSite_id($Site['Site']['_id']);
        foreach( $Site['Pages'] as &$Page )
        {
            $Page['Site_id'] = $Site['Site']['_id'];
            $P_id = $Page['Directives'];
            $P = $ps->Read($Page['Name']);
            if( !empty($P) )
                $ps->Delete($P);
            $ps->Create($Page);

            foreach( $P_id as $D )
                $ps->SetDirective($D,$Page);
        }

        $ts->UseSite_id($Site['Site']['_id']);
        foreach( $Site['Templates'] as &$Template )
        {
            $Template['Site_id'] = $Site['Site']['_id'];
            $T = $ts->Read($Template['Name']);
            if( !empty($T) )
                $ts->Delete($T);
            $ts->Create($Template);
        }

        foreach( $Site['Content'] as &$Content )
        {
            $Content['Site_id'] = $Site['Site']['_id'];
            $C = $cs->Match($Site['Site']['_id'],$Content['Path']);
            if( !empty($C) )
                $cs->Delete($Content);
            $cs->Create($Content);
        }
    }
}

