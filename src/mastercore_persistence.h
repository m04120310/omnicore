#ifndef MASTERCORE_PERSISTENCE_H
#define MASTERCORE_PERSISTENCE_H

#include "leveldb/db.h"

#include <boost/filesystem/path.hpp>

/** Base class for LevelDB based storage.
 */
class CDBBase
{
private:
    //! Options used when iterating over values of the database
    leveldb::ReadOptions iteroptions;

protected:
    //! Database options used
    leveldb::Options options;

    //! Options used when reading from the database
    leveldb::ReadOptions readoptions;

    //! Options used when writing to the database
    leveldb::WriteOptions writeoptions;

    //! Options used when sync writing to the database
    leveldb::WriteOptions syncoptions;

    //! The database itself
    leveldb::DB* pdb;

    //! Number of entries read
    unsigned int nRead;

    //! Number of entries written
    unsigned int nWritten;

    CDBBase() : nRead(0), nWritten(0)
    {
        options.paranoid_checks = true;
        options.create_if_missing = true;
        options.compression = leveldb::kNoCompression;
        options.max_open_files = 64;
        readoptions.verify_checksums = true;
        iteroptions.verify_checksums = true;
        iteroptions.fill_cache = false;
        syncoptions.sync = true;
    }

    virtual ~CDBBase()
    {
        Close();
    }

    leveldb::Iterator* NewIterator() const
    {
        return pdb->NewIterator(iteroptions);
    }

    /**
     * Opens or creates a LevelDB based database.
     *
     * If the database is wiped before opening, it's content is destroyed, including
     * all log files and meta data.
     *
     * @param path   The path of the database to open
     * @param fWipe  Whether to wipe the database before opening
     * @return A Status object, indicating success or failure
     */
    leveldb::Status Open(const boost::filesystem::path& path, bool fWipe = false);

public:
    /**
     * Deletes all entries of the database, and resets the counters.
     */
    void Clear();

    /**
     * Deinitializes and closes the database.
     */
    void Close();
};


#endif // MASTERCORE_PERSISTENCE_H