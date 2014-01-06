@@@ListProfiles
SELECT ProfileID,InsertDT,FirstName,LastName
  FROM Profile
 ORDER BY InsertDT DESC

@@@UpdateFirstName
UPDATE Profile SET FirstName=<?=$this($FirstName)?>
 WHERE ProfileID=<?=$this($ProfileID)?>

@@@ReadProfiles
SELECT ProfileID,FirstName,LastName
  FROM Profile
 WHERE ProfileID IN (<?=$this($ProfileIDs)?>)


@@@CreateProfileTable
CREATE TABLE `Profile` (
  `ProfileID` int(11) NOT NULL AUTO_INCREMENT,
  `InsertDT` datetime DEFAULT NULL,
  `Prefix` varchar(45) DEFAULT NULL,
  `FirstName` varchar(45) DEFAULT NULL,
  `LastName` varchar(45) DEFAULT NULL,
  `Suffix` varchar(45) DEFAULT NULL,
  `Address1` varchar(45) DEFAULT NULL,
  `Address2` varchar(45) DEFAULT NULL,
  `City` varchar(45) DEFAULT NULL,
  `State` char(2) DEFAULT NULL,
  `ZipCode` varchar(45) DEFAULT NULL,
  `Email` varchar(45) DEFAULT NULL,
  `Age` int(11) DEFAULT NULL,
  `PhoneNumber` varchar(45) DEFAULT NULL,
  `CCNumber` varchar(45) DEFAULT NULL,
  `IP` varchar(45) DEFAULT NULL,
  `SSN` varchar(45) DEFAULT NULL,
  `Description` varchar(500) DEFAULT NULL,
  `Username` varchar(45) DEFAULT NULL,
  `Password` varchar(45) DEFAULT NULL,
  `HAU` varchar(45) DEFAULT NULL,
  PRIMARY KEY (`ProfileID`)
)

@@@CreateFileUploadTable
CREATE TABLE `FileUpload` (
  `FileID` int(11) NOT NULL AUTO_INCREMENT,
  `R_ProfileID` int(11) DEFAULT NULL,
  `Filename` varchar(45) DEFAULT NULL,
  `FileSize` int(11) DEFAULT NULL,
  `ContentType` varchar(45) DEFAULT NULL,
  `FileData` blob,
  PRIMARY KEY (`FileID`)
)
