<?php
namespace asm;

// manage versions of sites in svn/git along with help from Pack
class Versioning
{
    protected $BranchURL = '';
    protected $TrunkURL = '';
    protected $Account_id = '';
    public $ss;
    public $ps;
    public $ts;
    public $cs;


    public function __construct( $BranchURL,$TrunkURL,\MongoId $Account_id,\asm\SiteSet $ss,\asm\PageSet $ps,\asm\TemplateSet $ts,\asm\ContentSet $cs )
    {
        $this->BranchURL = $BranchURL;
        $this->TrunkURL = $TrunkURL;
        $this->Account_id = $Account_id;

        $this->ss = $ss;
        $this->ps = $ps;
        $this->ts = $ts;
        $this->cs = $cs;
    }

    // completly reverts site to what's in svn - local changes are lost!
    public function svnRevert( \MongoId $Site_id )
    {
        $Site = $this->ss->Read($Site_id);
        if( empty($Site) )
            return FALSE;

        $exportDir = sys_get_temp_dir()."/{$Site['Domain']}_export_".time().'/';

        $this->svnExec("export {$this->BranchURL} {$exportDir}");

        $S = Pack::FromDir($exportDir,$Site['Domain'],$this->Account_id);

        $S['Site']['_id'] = $Site['_id'];

        $pack = new Pack($this->ss,$this->ps,$this->ts,$this->cs);
        $pack->ToMongo($S);

        // yup
        system("rm -rf {$exportDir}");
    }

    public function svnDiff( \MongoId $Site_id )
    {
        $Site = $this->ss->Read($Site_id);
        if( empty($Site) )
            return FALSE;

        $coDir = sys_get_temp_dir()."/{$Site['Domain']}_co_".time().'/';

        $this->svnExec("co {$this->BranchURL} {$coDir}");

        $pack = new Pack($this->ss,$this->ps,$this->ts,$this->cs);
        $asmS = $pack->FromMongo($Site['_id'],TRUE);

        // merge from mongo into the wc
        $this->ToSVNWC($asmS,$coDir);

        $diff = $this->svnExec("diff {$coDir}");
        $status = $this->svnExec("status {$coDir}");

        // yup
        system("rm -rf {$coDir}");

        return array($status,$diff);
    }

    // commit changes to svn
    // this uses a lot of RAM
    public function svnCommit( \MongoId $Site_id,$NoContent = FALSE )
    {
        $Site = $this->ss->Read($Site_id);
        if( empty($Site) )
            return FALSE;

        $coDir = sys_get_temp_dir()."/{$Site['Domain']}_co_".time();

        $this->svnExec("co {$this->BranchURL} {$coDir}");
        $this->svnExec("merge {$this->TrunkURL} {$coDir}");
        $this->svnExec("ci {$coDir} -m 'merge of trunk to {$Site['Domain']} branch'");

        // now merge pages/templates/content from our local site in mongo - svn add new files
        $pack = new Pack($this->ss,$this->ps,$this->ts,$this->cs);
        $asmS = $pack->FromMongo($Site['_id'],$NoContent);

        // save a snapshot backup just in case
        Pack::ToDir($asmS,$coDir."_snapshot");

        // merge from mongo into the wc
        $this->ToSVNWC($asmS,$coDir);

        $this->svnExec("ci {$coDir} -m 'asm local commit of {$Site['Domain']}'");

        // yup
        system("rm -rf {$coDir}");
    }

    protected function ToSVNWC( $Site,$WCPath )
    {
        if( !is_writeable($WCPath) )
            throw new Exception("Cannot write to '$WCPath'");

        // snapshot into memory to determine what's we may have deleted locally - not very efficient
        $BranchS = Pack::FromDir($WCPath,$Site['Site']['Domain'],$Site['Site']['AAID']);

        file_put_contents("{$WCPath}/Site.inc",Site::ToPHP($Site['Site']));

        foreach( $Site['Pages'] as $P )
        {
            $FN = "{$WCPath}/Pages/{$P['Name']}.inc";

            $Exists = is_file($FN);
            file_put_contents($FN,Page::ToPHP($P));

            if( !$Exists )
                $this->svnExec("add $FN");
        }

        foreach( $Site['Templates'] as $T )
        {
            $FN = "{$WCPath}/Templates/{$T['Name']}.inc";

            $Exists = is_file($FN);
            file_put_contents($FN,Template::ToPHP($T));

            if( !$Exists )
                $this->svnExec("add $FN");
        }

        // common problem of dir structures here too
        foreach( $Site['Content'] as $Name => $Body )
        {
            $FN = "{$WCPath}/Content/{$Name}.inc";

            $Exists = is_file($FN);
            file_put_content($FN,$Body);

            if( !$Exists )
                $this->svnExec("add $FN");
        }

        // delete items that were deleted locally
        foreach( $BranchS['Pages'] as $P )
            if( !isset($Site['Pages'][$P['Name']]) )
                $this->svnExec("rm {$WCPath}/Pages/{$P['Name']}.inc");

        foreach( $BranchS['Templates'] as $T )
            if( !isset($Site['Templates'][$T['Name']]) )
                $this->svnExec("rm {$WCPath}/Templates/{$T['Name']}.inc");

        foreach( $BranchS['Content'] as $Name => $Body )
            if( !isset($Site['Content'][$Name]) )
                $this->svnExec("rm {$WCPath}/Content/{$Name}.inc");
    }


    // yay - and no escaping so don't be dumb
    protected function svnExec( $Cmd )
    {
        $R = 0;
        $OP = array();
        exec("echo 't' | /usr/bin/svn --config-dir /tmp/svn.silly --config-option servers:global:store-auth-creds=no --username praxis --password st4ckw4r3 $Cmd 2>&1",$OP,$R);

        if( $R !== 0 )
            throw new Exception("svn {$Cmd} failed: ".implode("\n",$OP));
        else
            return $OP;
    }
}

