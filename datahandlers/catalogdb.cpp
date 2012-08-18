/***************************************************************************
                catalogDB.cpp  -  K Desktop Planetarium
                             -------------------
    begin                : 2012/03/08
    copyright            : (C) 2012 by Rishab Arora
    email                : ra.rishab@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "catalogdb.h"

bool CatalogDB::Initialize() {
    skydb_ = QSqlDatabase::addDatabase("QSQLITE", "skydb");
    QString dbfile = KStandardDirs::locateLocal("data", "skyobjects.db");
    QFile testdb(dbfile);
    bool first_run = false;
    if (!testdb.exists()) {
        kDebug()<< i18n("DSO DB does not exist!");
        first_run = true;
    }
    skydb_.setDatabaseName(dbfile);
    if (!skydb_.open()) {
           kWarning() << i18n("Unable to open DSO database file!");
           kWarning() << LastError();
    } else {
        kDebug() << i18n("Opened the DSO Database. Ready!");
        if (first_run == true) {
            //FirstRun();
        }
    }
    skydb_.close();
    return true;
}

CatalogDB::~CatalogDB() {
    skydb_.close();
    QSqlDatabase::removeDatabase("skydb");
}

QSqlError CatalogDB::LastError() {
    // error description is in QSqlError::text()
    return skydb_.lastError();
}

QStringList* CatalogDB::Catalogs() {
    RefreshCatalogList();
    return &catalog_list_;
}

void CatalogDB::RefreshCatalogList()
{  
    catalog_list_.clear();
    skydb_.open();
    QSqlTableModel catalog(0, skydb_);    
    catalog.setTable("Catalog");
    catalog.select();

    for (int i =0; i < catalog.rowCount(); ++i) {
        QSqlRecord record = catalog.record(i);
        // TODO(spacetime): complete list!
        QString name = record.value("Name").toString();
        catalog_list_.append(name);
//         QString author = record.value("Author").toString();
//         QString license = record.value("License").toString();
//         QString compiled_by = record.value("CompiledBy").toString();
//         QString prefix = record.value("Prefix").toString();
    }

    catalog.clear();
    skydb_.close();
}

bool CatalogDB::ParseCatalogInfoToDB( const QStringList &lines, QStringList &columns )
{

    bool foundDataColumns = false; //set to true if description of data columns found
    int ncol = 0;

    QStringList errs;
    QString catName, catPrefix, catColor, catFluxFreq, catFluxUnit;
    float catEpoch;
    bool showerrs = false;

    catName.clear();
    catPrefix.clear();
    catColor.clear();
    catFluxFreq.clear();
    catFluxUnit.clear();
    catEpoch = 0.;

    int i=0;
    for ( ; i < lines.size(); ++i ) {
        QString d( lines.at(i) ); //current data line
        if ( d.left(1) != "#" ) break;  //no longer in header!

        int iname      = d.indexOf( "# Name: " );
        int iprefix    = d.indexOf( "# Prefix: " );
        int icolor     = d.indexOf( "# Color: " );
        int iepoch     = d.indexOf( "# Epoch: " );
        int ifluxfreq  = d.indexOf( "# Flux Frequency: ");
        int ifluxunit  = d.indexOf( "# Flux Unit: ");

        if ( iname == 0 ) { //line contains catalog name
            iname = d.indexOf(":")+2;
            if ( catName.isEmpty() ) {
                catName = d.mid( iname );
            } else { //duplicate name in header
                if ( showerrs )
                    errs.append( i18n( "Parsing header: " ) +
                                 i18n( "Extra Name field in header: %1.  Will be ignored", d.mid(iname) ) );
            }
        } else if ( iprefix == 0 ) { //line contains catalog prefix
            iprefix = d.indexOf(":")+2;
            if ( catPrefix.isEmpty() ) {
                catPrefix = d.mid( iprefix );
            } else { //duplicate prefix in header
                if ( showerrs )
                    errs.append( i18n( "Parsing header: " ) +
                                 i18n( "Extra Prefix field in header: %1.  Will be ignored", d.mid(iprefix) ) );
            }
        } else if ( icolor == 0 ) { //line contains catalog prefix
            icolor = d.indexOf(":")+2;
            if ( catColor.isEmpty() ) {
                 catColor = d.mid( icolor );
            } else { //duplicate prefix in header
                if ( showerrs )
                    errs.append( i18n( "Parsing header: " ) +
                                 i18n( "Extra Color field in header: %1.  Will be ignored", d.mid(icolor) ) );
            }
        } else if ( iepoch == 0 ) { //line contains catalog epoch
            iepoch = d.indexOf(":")+2;
            if ( catEpoch == 0. ) {
                bool ok( false );
                 catEpoch = d.mid( iepoch ).toFloat( &ok );
                if ( !ok ) {
                    if ( showerrs )
                        errs.append( i18n( "Parsing header: " ) +
                                     i18n( "Could not convert Epoch to float: %1.  Using 2000. instead", d.mid(iepoch) ) );
                    catEpoch = 2000.; //adopt default value
                }
            }
        } else if ( ifluxfreq == 0 )
        { //line contains catalog flux frequnecy
            ifluxfreq = d.indexOf(":")+2;
            if ( catFluxFreq.isEmpty() )
            {
                catFluxFreq = d.mid( ifluxfreq );
            } else { //duplicate prefix in header
                if ( showerrs )
                    errs.append( i18n( "Parsing header: " ) +
                                 i18n( "Extra Flux Frequency field in header: %1.  Will be ignored", d.mid(ifluxfreq) ) );
            }
        } else if ( ifluxunit == 0 )
            { //line contains catalog flux unit
                       ifluxunit = d.indexOf(":")+2;
                       if ( catFluxUnit.isEmpty() )
                       {
                           catFluxUnit = d.mid( ifluxunit );
                       } else { //duplicate prefix in header
                           if ( showerrs )
                               errs.append( i18n( "Parsing header: " ) +
                                            i18n( "Extra Flux Unit field in header: %1.  Will be ignored", d.mid(ifluxunit) ) );
             }

        } else if ( ! foundDataColumns ) { //don't try to parse data column descriptors if we already found them
            //Chomp off leading "#" character
            d = d.remove( '#' );

            columns.clear();
            QStringList fields = d.split( ' ', QString::SkipEmptyParts ); //split on whitespace

            //we need a copy of the master list of data fields, so we can
            //remove fields from it as they are encountered in the "fields" list.
            //this allows us to detect duplicate entries
            QStringList master( columns );

            for ( int j=0; j < fields.size(); ++j ) {
                QString s( fields.at(j) );
                if ( master.contains( s ) ) {
                    //add the data field
                    columns.append( s );

                    // remove the field from the master list and inc the
                    // count of "good" columns (unless field is "Ignore")
                    if ( s != "Ig" ) {
                        master.removeAt( master.indexOf( s ) );
                        ncol++;
                    }
                } else if ( fields.contains( s ) ) { //duplicate field
                    fields.append( "Ig" ); //ignore the duplicate column
                    if ( showerrs )
                        errs.append( i18n( "Parsing header: " ) +
                                     i18n( "Duplicate data field descriptor \"%1\" will be ignored", s ) );
                } else { //Invalid field
                    fields.append( "Ig" ); //ignore the invalid column
                    if ( showerrs )
                        errs.append( i18n( "Parsing header: " ) +
                                     i18n( "Invalid data field descriptor \"%1\" will be ignored", s ) );
                }
            }

            if ( ncol ) foundDataColumns = true;
        }
    }

    if ( ! foundDataColumns ) {
        if ( showerrs )
            errs.append( i18n( "Parsing header: " ) +
                         i18n( "No valid column descriptors found.  Exiting" ) );
        return false;
    }

    if ( i == lines.size() ) {
        if ( showerrs ) errs.append( i18n( "Parsing header: " ) +
                                         i18n( "No data lines found after header.  Exiting." ) );
        return false;
    } else {
        //Make sure Name, Prefix, Color and Epoch were set
        if ( catName.isEmpty() ) {
            if ( showerrs ) errs.append( i18n( "Parsing header: " ) +
                                             i18n( "No Catalog Name specified; setting to \"Custom\"" ) );
            catName = i18n("Custom");
        }
        if ( catPrefix.isEmpty() ) {
            if ( showerrs ) errs.append( i18n( "Parsing header: " ) +
                                             i18n( "No Catalog Prefix specified; setting to \"CC\"" ) );
            catPrefix = "CC";
        }
        if ( catColor.isEmpty() ) {
            if ( showerrs ) errs.append( i18n( "Parsing header: " ) +
                                             i18n( "No Catalog Color specified; setting to Red" ) );
            catColor = "#CC0000";
        }
        if ( catEpoch == 0. ) {
            if ( showerrs ) errs.append( i18n( "Parsing header: " ) +
                                             i18n( "No Catalog Epoch specified; assuming 2000." ) );
            catEpoch = 2000.;
        }

        //index i now points to the first line past the header
//         iStart = i;
//         if ( catName
//         if ( KMessageBox::questionYesNo ( 0, message, errs,
//                         i18n( "Some Lines in File Were Invalid" ), KGuiItem( i18n( "Accept" ) ) ) != KMessageBox::Continue ) {
//                     m_ObjectList.clear();
//                     return ;
        return true;
    }
}

bool CatalogDB::AddCatalogContents(const QString& fname) {
    QDir::setCurrent( QDir::homePath() );  //for files with relative path
    QString filename = fname;
    //If the filename begins with "~", replace the "~" with the user's home directory
    //(otherwise, the file will not successfully open)
    if ( filename.at(0)=='~' )
        filename = QDir::homePath() + filename.mid( 1, filename.length() );

    QFile ccFile( filename );

    if ( ccFile.open( QIODevice::ReadOnly ) ) {
        int iStart(0); //the line number of the first non-header line
        QStringList errs; //list of error messages
        QStringList Columns; //list of data column descriptors in the header

        QTextStream stream( &ccFile );
        // TODO(spacetime) : Decide appropriate number of lines to be read
        QStringList lines;
        for (int times=5; times >= 0; --times)
          lines.append(stream.readLine());
        /*WAS
         * = stream.readAll().split( '\n', QString::SkipEmptyParts );
         * Memory Hog!
         */

        bool showerrs = false;
        if ( !ParseCatalogInfoToDB( lines, Columns) ) {
            kWarning() << "Incorrect header in catalog file: " << filename;
            return false;
        }
        // The entry in the Catalog table is now ready!
        
        /*
         * Now 'Columns' should be a StringList of the Header contents
         * Hence, we 1) Convert the Columns to a KSParser compatible format
         *           2) Use KSParser to read stuff and store in DB
         */
        
        //Part 1) Conversion to KSParser compatible format
        QList< QPair<QString, KSParser::DataTypes> > sequence = 
                                            buildParserSequence(Columns);
        KSParser catalog_text_parser(filename, '#', sequence);
        

        QHash<QString, QVariant> row_content;
        while (catalog_text_parser.HasNextRow()){
          row_content = catalog_text_parser.ReadNextRow();
        }
    }
    return true;
}


QList< QPair<QString, KSParser::DataTypes> > CatalogDB::
                              buildParserSequence(const QStringList& Columns)
{
    QList< QPair<QString, KSParser::DataTypes> > sequence;
    QStringList::const_iterator iter = Columns.begin();

    while (iter != Columns.end()) {
    // Available Types: ID RA Dc Tp Nm Mg Flux Mj Mn PA Ig
      KSParser::DataTypes current_type;

      if (*iter == QString("ID"))
          current_type = KSParser::D_QSTRING;
      else if (*iter == QString("RA"))
          current_type = KSParser::D_DOUBLE;
      else if (*iter == QString("Dc"))
          current_type = KSParser::D_DOUBLE;
      else if (*iter == QString("Tp"))
          current_type = KSParser::D_INT;
      else if (*iter == QString("Nm"))
          current_type = KSParser::D_QSTRING;
      else if (*iter == QString("Mg"))
          current_type = KSParser::D_FLOAT;
      else if (*iter == QString("Flux"))
          current_type = KSParser::D_FLOAT;
      else if (*iter == QString("Mj"))
          current_type = KSParser::D_FLOAT;
      else if (*iter == QString("Mn"))
          current_type = KSParser::D_FLOAT;
      else if (*iter == QString("PA"))
          current_type = KSParser::D_FLOAT;
      else if (*iter == QString("Ig"))
          current_type = KSParser::D_SKIP;

      sequence.append(qMakePair(*iter, current_type));
      ++iter;
    }

    return sequence;
}