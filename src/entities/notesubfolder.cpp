#include "entities/notesubfolder.h"
#include "notefolder.h"
#include <QDebug>
#include <QSqlRecord>
#include <QSqlError>
#include <QDir>
#include <QSettings>
#include <utils/misc.h>


NoteSubFolder::NoteSubFolder() {
    this->id = 0;
    this->parentId = 0;
}

int NoteSubFolder::getId() {
    return this->id;
}

int NoteSubFolder::getParentId() {
    return this->parentId;
}

NoteSubFolder NoteSubFolder::getParent() {
    return NoteSubFolder::fetch(parentId);
}

QString NoteSubFolder::getName() {
    return this->name;
}

QDateTime NoteSubFolder::getFileLastModified() {
    return this->fileLastModified;
}

QDateTime NoteSubFolder::getModified() {
    return this->modified;
}

void NoteSubFolder::setName(QString text) {
    this->name = text;
}

void NoteSubFolder::setParentId(int parentId) {
    this->parentId = parentId;
}

bool NoteSubFolder::isFetched() {
    return (this->id > 0);
}

NoteSubFolder NoteSubFolder::fetch(int id) {
    QSqlDatabase db = QSqlDatabase::database("memory");
    QSqlQuery query(db);

    NoteSubFolder noteSubFolder;

    query.prepare("SELECT * FROM noteSubFolder WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else {
        if (query.first()) {
            noteSubFolder = noteSubFolderFromQuery(query);
        }
    }

    return noteSubFolder;
}

NoteSubFolder NoteSubFolder::fetchByNameAndParentId(
        QString name, int parentId) {
    QSqlDatabase db = QSqlDatabase::database("memory");
    QSqlQuery query(db);

    NoteSubFolder noteSubFolder;

    query.prepare("SELECT * FROM noteSubFolder WHERE name = :name "
                          "AND parent_id = :parent_id");
    query.bindValue(":name", name);
    query.bindValue(":parent_id", parentId);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else {
        if (query.first()) {
            noteSubFolder = noteSubFolderFromQuery(query);
        }
    }

    return noteSubFolder;
}

/**
 * Gets the relative path name of the note sub folder
 */
QString NoteSubFolder::relativePath() {
    return parentId == 0 ?
           name :
           getParent().relativePath() + QDir::separator() + name;
}

/**
 * Gets the path data of the note sub folder
 */
QString NoteSubFolder::pathData() {
    return parentId == 0 ?
           name :
           getParent().relativePath() + "\n" + name;
}

/**
 * Fetches a note sub folder by it's path data
 */
NoteSubFolder NoteSubFolder::fetchByPathData(QString pathData) {
    QStringList pathList = pathData.split("\n");
    NoteSubFolder noteSubFolder;
    QStringListIterator itr(pathList);

    // loop through all names to fetch the deepest note sub folder
    while (itr.hasNext()) {
        QString name = itr.next();
        noteSubFolder = NoteSubFolder::fetchByNameAndParentId(
                name, noteSubFolder.getId());
        if (!noteSubFolder.isFetched()) {
            return NoteSubFolder();
        }
    }

    return noteSubFolder;
}

bool NoteSubFolder::remove() {
    QSqlDatabase db = QSqlDatabase::database("memory");
    QSqlQuery query(db);

    query.prepare("DELETE FROM noteSubFolder WHERE id = :id");
    query.bindValue(":id", this->id);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return false;
    } else {
        return true;
    }
}

NoteSubFolder NoteSubFolder::noteSubFolderFromQuery(QSqlQuery query) {
    NoteSubFolder noteSubFolder;
    noteSubFolder.fillFromQuery(query);
    return noteSubFolder;
}

bool NoteSubFolder::fillFromQuery(QSqlQuery query) {
    id = query.value("id").toInt();
    parentId = query.value("parent_id").toInt();
    name = query.value("name").toString();
    fileLastModified = query.value("file_last_modified").toDateTime();
    created = query.value("created").toDateTime();
    modified = query.value("modified").toDateTime();

    return true;
}

QList<NoteSubFolder> NoteSubFolder::fetchAll(int limit) {
    QSqlDatabase db = QSqlDatabase::database("memory");
    QSqlQuery query(db);

    QList<NoteSubFolder> noteSubFolderList;
    QString sql = "SELECT * FROM noteSubFolder "
            "ORDER BY file_last_modified DESC";

    if (limit >= 0) {
        sql += " LIMIT :limit";
    }

    query.prepare(sql);

    if (limit >= 0) {
        query.bindValue(":limit", limit);
    }

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else {
        for (int r = 0; query.next(); r++) {
            NoteSubFolder noteSubFolder = noteSubFolderFromQuery(query);
            noteSubFolderList.append(noteSubFolder);
        }
    }

    return noteSubFolderList;
}

QList<NoteSubFolder> NoteSubFolder::fetchAllByParentId(int parentId) {
    QSqlDatabase db = QSqlDatabase::database("memory");
    QSqlQuery query(db);

    QList<NoteSubFolder> noteSubFolderList;
    QString sql = "SELECT * FROM noteSubFolder WHERE parent_id = "
            ":parent_id ORDER BY file_last_modified DESC";

    query.prepare(sql);
    query.bindValue(":parent_id", parentId);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else {
        for (int r = 0; query.next(); r++) {
            NoteSubFolder noteSubFolder = noteSubFolderFromQuery(query);
            noteSubFolderList.append(noteSubFolder);
        }
    }

    return noteSubFolderList;
}

//
// inserts or updates a noteSubFolder object in the database
//
bool NoteSubFolder::store() {
    QSqlDatabase db = QSqlDatabase::database("memory");
    QSqlQuery query(db);

    // don't store noteSubFolders with empty name
    if (name.isEmpty()) {
        return false;
    }

    if (id > 0) {
        query.prepare("UPDATE noteSubFolder SET "
                              "parent_id = :parent_id,"
                              "name = :name,"
                              "file_last_modified = :file_last_modified,"
                              "modified = :modified "
                              "WHERE id = :id");
        query.bindValue(":id", id);
    } else {
        query.prepare("INSERT INTO noteSubFolder"
                              "(name, file_last_modified, parent_id,"
                              "modified) "
                              "VALUES (:name, :file_last_modified, :parent_id,"
                              ":modified)");
    }

    QDateTime modified = QDateTime::currentDateTime();

    query.bindValue(":name", name);
    query.bindValue(":parent_id", parentId);
    query.bindValue(":file_last_modified", fileLastModified);
    query.bindValue(":modified", modified);

    // on error
    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return false;
    } else if (id == 0) {  // on insert
        id = query.lastInsertId().toInt();
    }

    modified = modified;
    return true;
}

//
// deletes all noteSubFolders in the database
//
bool NoteSubFolder::deleteAll() {
    QSqlDatabase db = QSqlDatabase::database("memory");
    QSqlQuery query(db);

    // no truncate in sqlite
    query.prepare("DELETE FROM noteSubFolder");
    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return false;
    } else {
        return true;
    }
}

//
// checks if the current noteSubFolder still exists in the database
//
bool NoteSubFolder::exists() {
    NoteSubFolder noteSubFolder = NoteSubFolder::fetch(this->id);
    return noteSubFolder.id > 0;
}

/**
 * Counts all notes
 */
int NoteSubFolder::countAll() {
    QSqlDatabase db = QSqlDatabase::database("memory");
    QSqlQuery query(db);

    query.prepare("SELECT COUNT(*) AS cnt FROM noteSubFolder");

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else if (query.first()) {
        return query.value("cnt").toInt();
    }

    return 0;
}

void NoteSubFolder::setAsActive() {
    NoteSubFolder::setAsActive(id);
}

/**
 * Set a note sub folder as active note sub folder for the current note folder
 */
bool NoteSubFolder::setAsActive(int noteSubFolderId) {
    NoteFolder noteFolder = NoteFolder::currentNoteFolder();
    if (!noteFolder.isFetched()) {
        return false;
    }

    // we don't need to check if the note sub folder was really fetched
    // because we also want to get the root folder
    NoteSubFolder noteSubFolder = NoteSubFolder::fetch(noteSubFolderId);
    noteFolder.setActiveNoteSubFolder(noteSubFolder);
    return noteFolder.store();
}

/**
 * Checks if this note sub folder is the current one
 */
bool NoteSubFolder::isActive() {
    return activeSubNoteFolderId() == id;
}

/**
 * Returns the id of the current note sub folder of the current note folder
 */
int NoteSubFolder::activeSubNoteFolderId() {
    return activeNoteSubFolder().getId();
}

/**
 * Returns the current note sub folder of the current note folder
 */
NoteSubFolder NoteSubFolder::activeNoteSubFolder() {
    // we don't need to check if the note sub folder was really fetched
    // because we also want to get the root folder
    NoteFolder noteFolder = NoteFolder::currentNoteFolder();
    return noteFolder.getActiveNoteSubFolder();
}

QDebug operator<<(QDebug dbg, const NoteSubFolder &noteSubFolder) {
    dbg.nospace() << "NoteSubFolder: <id>" << noteSubFolder.id <<
            " <name>" << noteSubFolder.name <<
            " <parentId>" << noteSubFolder.parentId;
    return dbg.space();
}
