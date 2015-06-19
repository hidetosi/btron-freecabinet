typedef VOIDPTR DOCITERATOR; /* âªêgÇ–Ç∆Ç¬Çé¶Ç∑ï® */

extern BOOL AddVobjByID();
extern BOOL FileOpen();
extern BOOL ReadFile();
extern BOOL SaveFile();
extern BOOL SaveNewFile();

extern BOOL IsDataModified();
extern BOOL IsSegmentOnlyModified();
extern int SaveModification();
extern void RecoverModification();
extern void ForceNoSave();
extern BOOL IsFusen();
extern DOCITERATOR GetIterator();
extern int GetVobjNum();
extern DOCITERATOR GetNext();
extern DOCITERATOR GetPrevious();
extern WORD GetVID();
extern RECT GetRect();
extern void ChangeRect();
extern RECT GetDocRect();
extern int GetSelectedVobjNum();
extern BOOL IsFixed();
extern BOOL IsBackground();
extern void UnBackgroundAllVobj();
extern void NormalizeSelectedVobj();
extern void FixSelectedVobj();
extern void BackgroundVobj();
extern DOCITERATOR GetNextSelectedVobj();
extern BOOL IsSelected();
extern void SwitchVobjSelect();
extern void SelectVobj();
extern void UnSelectVobj();
extern void UnSelectAllVobj();
extern void SelectAllVobj();
extern void VobjSegmentModified();
extern void ChangedVirtualObjRelation();
extern void DeleteData();
extern void TakeFrontOrBackSelectedData();
extern POINT SendSelectedDataToTray();
extern void PasteFromTray();

extern BOOL ReadUniqueData();
extern void WriteUniqueData();
