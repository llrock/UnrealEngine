UPDATE Buggs 
SET [SummaryV2] = '' 
WHERE [SummaryV2] = 'KERNELBASE.dll'

UPDATE Crashes 
SET [Summary] = '' 
WHERE [Summary] = 'KERNELBASE.dll'

UPDATE Crashes 
SET [Summary] = ''
WHERE [Summary] = 'AssertLog='

UPDATE Crashes 
SET [Summary] = ''
WHERE [Summary] = 'Fatal error!' 

UPDATE Crashes 
SET [Description] = ''
WHERE [Description] = 'No comment provided' 



--OUTPUT $action, Inserted.*, Deleted.*