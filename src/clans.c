/***************************************************************************
*                           STAR WARS REALITY 1.0                          *
*--------------------------------------------------------------------------*
* Star Wars Reality Code Additions and changes from the Smaug Code         *
* copyright (c) 1997 by Sean Cooper                                        *
* -------------------------------------------------------------------------*
* Starwars and Starwars Names copyright(c) Lucas Film Ltd.                 *
*--------------------------------------------------------------------------*
* SMAUG 1.0 (C) 1994, 1995, 1996 by Derek Snider                           *
* SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,                    *
* Scryn, Rennard, Swordbearer, Gorog, Grishnakh and Tricops                *
* ------------------------------------------------------------------------ *
* Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
* Chastain, Michael Quan, and Mitchell Tse.                                *
* Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
* Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
* ------------------------------------------------------------------------ *
*			     Special clan module			    *
****************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mud.h"

#define MAX_NEST	100
static OBJ_DATA *rgObjNest[MAX_NEST];

CLAN_DATA *first_clan;
CLAN_DATA *last_clan;

SENATE_DATA *first_senator;
SENATE_DATA *last_senator;

PLANET_DATA *first_planet;
PLANET_DATA *last_planet;

GUARD_DATA *first_guard;
GUARD_DATA *last_guard;

/* local routines */
void fread_clan( CLAN_DATA * clan, FILE * fp );
bool load_clan_file( const char *clanfile );
void write_clan_list( void );
void fread_planet( PLANET_DATA * planet, FILE * fp );
bool load_planet_file( const char *planetfile );
void write_planet_list( void );

/*
 * Get pointer to clan structure from clan name.
 */
CLAN_DATA *get_clan( const char *name )
{
   CLAN_DATA *clan;

   for( clan = first_clan; clan; clan = clan->next )
      if( !str_cmp( name, clan->name ) )
         return clan;
   return NULL;
}

PLANET_DATA *get_planet( const char *name )
{
   PLANET_DATA *planet;

   for( planet = first_planet; planet; planet = planet->next )
      if( !str_cmp( name, planet->name ) )
         return planet;
   return NULL;
}

void write_clan_list(  )
{
   CLAN_DATA *tclan;
   FILE *fpout;
   char filename[256];

   sprintf( filename, "%s%s", CLAN_DIR, CLAN_LIST );
   fpout = fopen( filename, "w" );
   if( !fpout )
   {
      bug( "FATAL: cannot open clan.lst for writing!\r\n", 0 );
      return;
   }
   for( tclan = first_clan; tclan; tclan = tclan->next )
      fprintf( fpout, "%s\n", tclan->filename );
   fprintf( fpout, "$\n" );
   fclose( fpout );
}

void write_planet_list(  )
{
   PLANET_DATA *tplanet;
   FILE *fpout;
   char filename[256];

   sprintf( filename, "%s%s", PLANET_DIR, PLANET_LIST );
   fpout = fopen( filename, "w" );
   if( !fpout )
   {
      bug( "FATAL: cannot open planet.lst for writing!\r\n", 0 );
      return;
   }
   for( tplanet = first_planet; tplanet; tplanet = tplanet->next )
      fprintf( fpout, "%s\n", tplanet->filename );
   fprintf( fpout, "$\n" );
   fclose( fpout );
}

/*
 * Save a clan's data to its data file
 */
void save_clan( CLAN_DATA * clan )
{
   FILE *fp;
   char filename[256];

   if( !clan )
   {
      bug( "save_clan: null clan pointer!", 0 );
      return;
   }

   if( !clan->filename || clan->filename[0] == '\0' )
   {
      bug( "save_clan: %s has no filename", clan->name );
      return;
   }

   sprintf( filename, "%s%s", CLAN_DIR, clan->filename );

   if( ( fp = fopen( filename, "w" ) ) == NULL )
   {
      bug( "save_clan: fopen", 0 );
      perror( filename );
   }
   else
   {
      fprintf( fp, "#CLAN\n" );
      fprintf( fp, "Name         %s~\n", clan->name );
      fprintf( fp, "Filename     %s~\n", clan->filename );
      fprintf( fp, "Description  %s~\n", clan->description );
      fprintf( fp, "Leader       %s~\n", clan->leader );
      fprintf( fp, "NumberOne    %s~\n", clan->number1 );
      fprintf( fp, "NumberTwo    %s~\n", clan->number2 );
      fprintf( fp, "PKills       %d\n", clan->pkills );
      fprintf( fp, "PDeaths      %d\n", clan->pdeaths );
      fprintf( fp, "MKills       %d\n", clan->mkills );
      fprintf( fp, "MDeaths      %d\n", clan->mdeaths );
      fprintf( fp, "Type         %d\n", clan->clan_type );
      fprintf( fp, "Members      %d\n", clan->members );
      fprintf( fp, "Board        %d\n", clan->board );
      fprintf( fp, "Storeroom    %d\n", clan->storeroom );
      fprintf( fp, "GuardOne     %d\n", clan->guard1 );
      fprintf( fp, "GuardTwo     %d\n", clan->guard2 );
      fprintf( fp, "PatrolOne    %d\n", clan->patrol1 );
      fprintf( fp, "PatrolTwo    %d\n", clan->patrol2 );
      fprintf( fp, "TrooperOne   %d\n", clan->trooper1 );
      fprintf( fp, "TrooperTwo   %d\n", clan->trooper2 );
      fprintf( fp, "Funds        %ld\n", clan->funds );
      fprintf( fp, "Jail         %d\n", clan->jail );
      if( clan->mainclan )
         fprintf( fp, "MainClan     %s~\n", clan->mainclan->name );
      fprintf( fp, "End\n\n" );
      fprintf( fp, "#END\n" );
      fclose( fp );
      fp = NULL;
   }
   return;
}

void save_planet( PLANET_DATA * planet )
{
   FILE *fp;
   char filename[256];

   if( !planet )
   {
      bug( "save_planet: null planet pointer!", 0 );
      return;
   }

   if( !planet->filename || planet->filename[0] == '\0' )
   {
      bug( "save_planet: %s has no filename", planet->name );
      return;
   }

   sprintf( filename, "%s%s", PLANET_DIR, planet->filename );

   if( ( fp = fopen( filename, "w" ) ) == NULL )
   {
      bug( "save_planet: fopen", 0 );
      perror( filename );
   }
   else
   {
      AREA_DATA *pArea;

      fprintf( fp, "#PLANET\n" );
      fprintf( fp, "Name         %s~\n", planet->name );
      fprintf( fp, "Filename     %s~\n", planet->filename );
      fprintf( fp, "BaseValue    %ld\n", planet->base_value );
      fprintf( fp, "Flags        %d\n", planet->flags );
      fprintf( fp, "PopSupport   %f\n", planet->pop_support );
      if( planet->starsystem && planet->starsystem->name )
         fprintf( fp, "Starsystem   %s~\n", planet->starsystem->name );
      if( planet->governed_by && planet->governed_by->name )
         fprintf( fp, "GovernedBy   %s~\n", planet->governed_by->name );
      for( pArea = planet->first_area; pArea; pArea = pArea->next_on_planet )
         if( pArea->filename )
            fprintf( fp, "Area         %s~\n", pArea->filename );
      fprintf( fp, "End\n\n" );
      fprintf( fp, "#END\n" );
      fclose( fp );
      fp = NULL;
   }
   return;
}

/*
 * Read in actual clan data.
 */
void fread_clan( CLAN_DATA * clan, FILE * fp )
{
   char buf[MAX_STRING_LENGTH];
   const char *word;
   bool fMatch;

   for( ;; )
   {
      word = feof( fp ) ? "End" : fread_word( fp );
      fMatch = FALSE;

      switch ( UPPER( word[0] ) )
      {
         case '*':
            fMatch = TRUE;
            fread_to_eol( fp );
            break;

         case 'B':
            KEY( "Board", clan->board, fread_number( fp ) );
            break;

         case 'D':
            KEY( "Description", clan->description, fread_string( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( !clan->name )
                  clan->name = STRALLOC( "" );
               if( !clan->leader )
                  clan->leader = STRALLOC( "" );
               if( !clan->description )
                  clan->description = STRALLOC( "" );
               if( !clan->number1 )
                  clan->number1 = STRALLOC( "" );
               if( !clan->number2 )
                  clan->number2 = STRALLOC( "" );
               if( !clan->tmpstr )
                  clan->tmpstr = STRALLOC( "" );
               return;
            }
            break;

         case 'F':
            KEY( "Funds", clan->funds, fread_number( fp ) );
            KEY( "Filename", clan->filename, fread_string_nohash( fp ) );
            break;

         case 'G':
            KEY( "GuardOne", clan->guard1, fread_number( fp ) );
            KEY( "GuardTwo", clan->guard2, fread_number( fp ) );
            break;

         case 'J':
            KEY( "Jail", clan->jail, fread_number( fp ) );
            break;

         case 'L':
            KEY( "Leader", clan->leader, fread_string( fp ) );
            break;

         case 'M':
            KEY( "MDeaths", clan->mdeaths, fread_number( fp ) );
            KEY( "Members", clan->members, fread_number( fp ) );
            KEY( "MKills", clan->mkills, fread_number( fp ) );
            KEY( "MainClan", clan->tmpstr, fread_string( fp ) );
            break;

         case 'N':
            KEY( "Name", clan->name, fread_string( fp ) );
            KEY( "NumberOne", clan->number1, fread_string( fp ) );
            KEY( "NumberTwo", clan->number2, fread_string( fp ) );
            break;

         case 'P':
            KEY( "PDeaths", clan->pdeaths, fread_number( fp ) );
            KEY( "PKills", clan->pkills, fread_number( fp ) );
            KEY( "PatrolOne", clan->patrol1, fread_number( fp ) );
            KEY( "PatrolTwo", clan->patrol2, fread_number( fp ) );
            break;

         case 'S':
            KEY( "Storeroom", clan->storeroom, fread_number( fp ) );
            break;

         case 'T':
            KEY( "Type", clan->clan_type, fread_number( fp ) );
            KEY( "TrooperOne", clan->trooper1, fread_number( fp ) );
            KEY( "TrooperTwo", clan->trooper2, fread_number( fp ) );
            break;
      }

      if( !fMatch )
      {
         sprintf( buf, "Fread_clan: no match: %s", word );
         bug( buf, 0 );
      }

   }
}

void fread_planet( PLANET_DATA * planet, FILE * fp )
{
   char buf[MAX_STRING_LENGTH];
   const char *word;
   bool fMatch;

   for( ;; )
   {
      word = feof( fp ) ? "End" : fread_word( fp );
      fMatch = FALSE;

      switch ( UPPER( word[0] ) )
      {
         case '*':
            fMatch = TRUE;
            fread_to_eol( fp );
            break;

         case 'A':
            if( !str_cmp( word, "Area" ) )
            {
	      const char *aName;
               AREA_DATA *pArea;

               aName = fread_string( fp );
               for( pArea = first_area; pArea; pArea = pArea->next )
                  if( pArea->filename && !str_cmp( pArea->filename, aName ) )
                  {
                     pArea->planet = planet;
                     LINK( pArea, planet->first_area, planet->last_area, next_on_planet, prev_on_planet );
                  }
               fMatch = TRUE;
               STRFREE( aName );
            }
            break;

         case 'B':
            KEY( "BaseValue", planet->base_value, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( !planet->name )
                  planet->name = STRALLOC( "" );
               return;
            }
            break;

         case 'F':
            KEY( "Filename", planet->filename, fread_string_nohash( fp ) );
            KEY( "Flags", planet->flags, fread_number( fp ) );
            break;

         case 'G':
            if( !str_cmp( word, "GovernedBy" ) )
            {
               const char *clan_name = fread_string( fp );
               planet->governed_by = get_clan( clan_name );
               fMatch = TRUE;
               STRFREE( clan_name );
            }
            break;


         case 'N':
            KEY( "Name", planet->name, fread_string( fp ) );
            break;

         case 'P':
            KEY( "PopSupport", planet->pop_support, fread_float( fp ) );
            break;

         case 'S':
            if( !str_cmp( word, "Starsystem" ) )
            {
               const char *starsystem_name = fread_string( fp );
               planet->starsystem = starsystem_from_name( starsystem_name );
               if( planet->starsystem )
               {
                  SPACE_DATA *starsystem = planet->starsystem;

                  LINK( planet, starsystem->first_planet, starsystem->last_planet, next_in_system, prev_in_system );
               }
               fMatch = TRUE;
               STRFREE( starsystem_name );
            }
            break;

         case 'T':
            KEY( "Taxes", planet->base_value, fread_number( fp ) );
            break;

      }

      if( !fMatch )
      {
         sprintf( buf, "Fread_planet: no match: %s", word );
         bug( buf, 0 );
      }
   }
}

/*
 * Load a clan file
 */
bool load_clan_file( const char *clanfile )
{
   char filename[256];
   CLAN_DATA *clan;
   FILE *fp;
   bool found;

   CREATE( clan, CLAN_DATA, 1 );
   clan->next_subclan = NULL;
   clan->prev_subclan = NULL;
   clan->last_subclan = NULL;
   clan->first_subclan = NULL;
   clan->mainclan = NULL;

   found = FALSE;
   sprintf( filename, "%s%s", CLAN_DIR, clanfile );

   if( ( fp = fopen( filename, "r" ) ) != NULL )
   {
      found = TRUE;
      for( ;; )
      {
         char letter;
         const char *word;

         letter = fread_letter( fp );
         if( letter == '*' )
         {
            fread_to_eol( fp );
            continue;
         }

         if( letter != '#' )
         {
            bug( "Load_clan_file: # not found.", 0 );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "CLAN" ) )
         {
            fread_clan( clan, fp );
            break;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            char buf[MAX_STRING_LENGTH];

            sprintf( buf, "Load_clan_file: bad section: %s.", word );
            bug( buf, 0 );
            break;
         }
      }
      fclose( fp );
   }

   if( found )
   {
      ROOM_INDEX_DATA *storeroom;

      LINK( clan, first_clan, last_clan, next, prev );

      if( clan->storeroom == 0 || ( storeroom = get_room_index( clan->storeroom ) ) == NULL )
      {
         log_string( "Storeroom not found" );
         return found;
      }

      sprintf( filename, "%s%s.vault", CLAN_DIR, clan->filename );
      if( ( fp = fopen( filename, "r" ) ) != NULL )
      {
         int iNest;
         OBJ_DATA *tobj, *tobj_next;

         log_string( "Loading clan storage room" );
         rset_supermob( storeroom );
         for( iNest = 0; iNest < MAX_NEST; iNest++ )
            rgObjNest[iNest] = NULL;

         for( ;; )
         {
            char letter;
            const char *word;

            letter = fread_letter( fp );
            if( letter == '*' )
            {
               fread_to_eol( fp );
               continue;
            }

            if( letter != '#' )
            {
               bug( "Load_clan_vault: # not found.", 0 );
               bug( clan->name, 0 );
               break;
            }

            word = fread_word( fp );
            if( !str_cmp( word, "OBJECT" ) ) /* Objects  */
               fread_obj( supermob, fp, OS_CARRY );
            else if( !str_cmp( word, "END" ) )  /* Done     */
               break;
            else
            {
               bug( "Load_clan_vault: bad section.", 0 );
               bug( clan->name, 0 );
               break;
            }
         }
         fclose( fp );
         for( tobj = supermob->first_carrying; tobj; tobj = tobj_next )
         {
            tobj_next = tobj->next_content;
            obj_from_char( tobj );
            obj_to_room( tobj, storeroom );
         }
         release_supermob(  );
      }
      else
         log_string( "Cannot open clan vault" );
   }
   else
      DISPOSE( clan );

   return found;
}

bool load_planet_file( const char *planetfile )
{
   char filename[256];
   PLANET_DATA *planet;
   FILE *fp;
   bool found;

   CREATE( planet, PLANET_DATA, 1 );

   planet->governed_by = NULL;
   planet->next_in_system = NULL;
   planet->prev_in_system = NULL;
   planet->starsystem = NULL;
   planet->first_area = NULL;
   planet->last_area = NULL;
   planet->first_guard = NULL;
   planet->last_guard = NULL;

   found = FALSE;
   sprintf( filename, "%s%s", PLANET_DIR, planetfile );

   if( ( fp = fopen( filename, "r" ) ) != NULL )
   {

      found = TRUE;
      for( ;; )
      {
         char letter;
         const char *word;

         letter = fread_letter( fp );
         if( letter == '*' )
         {
            fread_to_eol( fp );
            continue;
         }

         if( letter != '#' )
         {
            bug( "Load_planet_file: # not found.", 0 );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "PLANET" ) )
         {
            fread_planet( planet, fp );
            break;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            char buf[MAX_STRING_LENGTH];

            sprintf( buf, "Load_planet_file: bad section: %s.", word );
            bug( buf, 0 );
            break;
         }
      }
      fclose( fp );
   }

   if( !found )
      DISPOSE( planet );
   else
      LINK( planet, first_planet, last_planet, next, prev );

   return found;
}

/*
 * Load in all the clan files.
 */
void load_clans()
{
   FILE *fpList;
   const char *filename;
   char clanlist[256];
   char buf[MAX_STRING_LENGTH];
   CLAN_DATA *clan;
   CLAN_DATA *bosclan;

   first_clan = NULL;
   last_clan = NULL;

   log_string( "Loading clans..." );

   sprintf( clanlist, "%s%s", CLAN_DIR, CLAN_LIST );
   if( ( fpList = fopen( clanlist, "r" ) ) == NULL )
   {
      perror( clanlist );
      exit( 1 );
   }

   for( ;; )
   {
      filename = feof( fpList ) ? "$" : fread_word( fpList );
      log_string( filename );
      if( filename[0] == '$' )
         break;

      if( !load_clan_file( filename ) )
      {
         sprintf( buf, "Cannot load clan file: %s", filename );
         bug( buf, 0 );
      }
   }
   fclose( fpList );
   log_string( " Done clans\r\nSorting clans...." );

   for( clan = first_clan; clan; clan = clan->next )
   {
      if( !clan->tmpstr || clan->tmpstr[0] == '\0' )
         continue;

      bosclan = get_clan( clan->tmpstr );
      if( !bosclan )
         continue;

      LINK( clan, bosclan->first_subclan, bosclan->last_subclan, next_subclan, prev_subclan );
      clan->mainclan = bosclan;
   }

   log_string( " Done sorting" );
   return;
}

void load_planets()
{
   FILE *fpList;
   const char *filename;
   char planetlist[256];
   char buf[MAX_STRING_LENGTH];

   first_planet = NULL;
   last_planet = NULL;

   log_string( "Loading planets..." );

   sprintf( planetlist, "%s%s", PLANET_DIR, PLANET_LIST );
   if( ( fpList = fopen( planetlist, "r" ) ) == NULL )
   {
      perror( planetlist );
      exit( 1 );
   }

   for( ;; )
   {
      filename = feof( fpList ) ? "$" : fread_word( fpList );
      log_string( filename );
      if( filename[0] == '$' )
         break;

      if( !load_planet_file( filename ) )
      {
         sprintf( buf, "Cannot load planet file: %s", filename );
         bug( buf, 0 );
      }
   }
   fclose( fpList );
   log_string( " Done planets " );
   return;
}

void do_make( CHAR_DATA * ch, const char *argument )
{
   send_to_char( "Huh?\r\n", ch );
   return;
}

void do_induct( CHAR_DATA * ch, const char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   CLAN_DATA *clan;

   if( IS_NPC( ch ) || !ch->pcdata->clan )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   clan = ch->pcdata->clan;

   if( ( ch->pcdata && ch->pcdata->bestowments
         && is_name( "induct", ch->pcdata->bestowments ) )
       || !str_cmp( ch->name, clan->leader ) || !str_cmp( ch->name, clan->number1 ) || !str_cmp( ch->name, clan->number2 ) )
      ;
   else
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      send_to_char( "Induct whom?\r\n", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "That player is not here.\r\n", ch );
      return;
   }

   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\r\n", ch );
      return;
   }

   if( victim->pcdata->clan )
   {
      if( victim->pcdata->clan->clan_type == CLAN_CRIME )
      {
         if( victim->pcdata->clan == clan )
            send_to_char( "This player already belongs to your crime family!\r\n", ch );
         else
            send_to_char( "This player already belongs to an organization!\r\n", ch );
         return;
      }
      else if( victim->pcdata->clan->clan_type == CLAN_GUILD )
      {
         if( victim->pcdata->clan == clan )
            send_to_char( "This player already belongs to your guild!\r\n", ch );
         else
            send_to_char( "This player already belongs to an organization!\r\n", ch );
         return;
      }
      else
      {
         if( victim->pcdata->clan == clan )
            send_to_char( "This player already belongs to your organization!\r\n", ch );
         else
            send_to_char( "This player already belongs to an organization!\r\n", ch );
         return;
      }

   }

   clan->members++;

   victim->pcdata->clan = clan;
   STRFREE( victim->pcdata->clan_name );
   victim->pcdata->clan_name = QUICKLINK( clan->name );
   act( AT_MAGIC, "You induct $N into $t", ch, clan->name, victim, TO_CHAR );
   act( AT_MAGIC, "$n inducts $N into $t", ch, clan->name, victim, TO_NOTVICT );
   act( AT_MAGIC, "$n inducts you into $t", ch, clan->name, victim, TO_VICT );
   save_char_obj( victim );
   return;
}

/* Can the character outcast the victim? */
bool can_outcast( CLAN_DATA *clan, CHAR_DATA *ch, CHAR_DATA *victim )
{
   if( !clan || !ch || !victim )
      return FALSE;
   if( !str_cmp( ch->name, clan->leader ) )
      return TRUE;
   if( !str_cmp( victim->name, clan->leader ) )
      return FALSE;
   if( !str_cmp( ch->name, clan->number1 ) )
      return TRUE;
   if( !str_cmp( victim->name, clan->number1 ) )
      return FALSE;
   if( !str_cmp( ch->name, clan->number2 ) )
      return TRUE;
   if( !str_cmp( victim->name, clan->number2 ) )
      return FALSE;
   return TRUE;
}

void do_outcast( CHAR_DATA * ch, const char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   CLAN_DATA *clan;
   char buf[MAX_STRING_LENGTH];

   if( IS_NPC( ch ) || !ch->pcdata->clan )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   clan = ch->pcdata->clan;

   if( ( ch->pcdata && ch->pcdata->bestowments
         && is_name( "outcast", ch->pcdata->bestowments ) )
       || !str_cmp( ch->name, clan->leader ) || !str_cmp( ch->name, clan->number1 ) || !str_cmp( ch->name, clan->number2 ) )
      ;
   else
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      send_to_char( "Outcast whom?\r\n", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "That player is not here.\r\n", ch );
      return;
   }

   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "Kick yourself out of your own clan?\r\n", ch );
      return;
   }

   if( victim->pcdata->clan != ch->pcdata->clan )
   {
      send_to_char( "This player does not belong to your clan!\r\n", ch );
      return;
   }

   if( !can_outcast( clan, ch, victim ) )
   {
      send_to_char( "You are not able to outcast them.\r\n", ch );
      return;
   }

   if( victim->speaking & LANG_CLAN )
      victim->speaking = LANG_COMMON;
   REMOVE_BIT( victim->speaks, LANG_CLAN );
   --clan->members;
    if( clan->members < 0 )
       clan->members = 0;
   if( !str_cmp( victim->name, ch->pcdata->clan->number1 ) )
   {
      STRFREE( ch->pcdata->clan->number1 );
      ch->pcdata->clan->number1 = STRALLOC( "" );
   }
   if( !str_cmp( victim->name, ch->pcdata->clan->number2 ) )
   {
      STRFREE( ch->pcdata->clan->number2 );
      ch->pcdata->clan->number2 = STRALLOC( "" );
   }
   victim->pcdata->clan = NULL;
   STRFREE( victim->pcdata->clan_name );
   victim->pcdata->clan_name = STRALLOC( "" );
   act( AT_MAGIC, "You outcast $N from $t", ch, clan->name, victim, TO_CHAR );
   act( AT_MAGIC, "$n outcasts $N from $t", ch, clan->name, victim, TO_ROOM );
   act( AT_MAGIC, "$n outcasts you from $t", ch, clan->name, victim, TO_VICT );
   sprintf( buf, "%s has been outcast from %s!", victim->name, clan->name );
   echo_to_all( AT_MAGIC, buf, ECHOTAR_ALL );

   DISPOSE( victim->pcdata->bestowments );
   victim->pcdata->bestowments = str_dup( "" );

   save_char_obj( victim );   /* clan gets saved when pfile is saved */
   return;
}

void do_setclan( CHAR_DATA * ch, const char *argument )
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   CLAN_DATA *clan;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' )
   {
      send_to_char( "Usage: setclan <clan> <field> <leader|number1|number2> <player>\r\n", ch );
      send_to_char( "\r\nField being one of:\r\n", ch );
      send_to_char( " leader number1 number2 subclan\r\n", ch );
      send_to_char( " members board recall storage\r\n", ch );
      send_to_char( " funds trooper1 trooper2 jail", ch );
      send_to_char( " guard1 guard2 patrol1 patrol2\r\n", ch );
      if( get_trust( ch ) >= LEVEL_SUB_IMPLEM )
      {
         send_to_char( " name filename desc\r\n", ch );
      }
      return;
   }

   clan = get_clan( arg1 );
   if( !clan )
   {
      send_to_char( "No such clan.\r\n", ch );
      return;
   }

   if( !strcmp( arg2, "leader" ) )
   {
      STRFREE( clan->leader );
      clan->leader = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_clan( clan );
      return;
   }

   if( !strcmp( arg2, "subclan" ) )
   {
      CLAN_DATA *subclan;
      subclan = get_clan( argument );
      if( !subclan )
      {
         send_to_char( "Subclan is not a clan.\r\n", ch );
         return;
      }
      if( subclan->clan_type == CLAN_SUBCLAN || subclan->mainclan )
      {
         send_to_char( "Subclan is already part of another organization.\r\n", ch );
         return;
      }
      if( subclan->first_subclan )
      {
         send_to_char( "Subclan has subclans of its own that need removing first.\r\n", ch );
         return;
      }
      subclan->clan_type = CLAN_SUBCLAN;
      subclan->mainclan = clan;
      LINK( subclan, clan->first_subclan, clan->last_subclan, next_subclan, prev_subclan );
      save_clan( clan );
      save_clan( subclan );
      return;
   }

   if( !strcmp( arg2, "number1" ) )
   {
      STRFREE( clan->number1 );
      clan->number1 = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_clan( clan );
      return;
   }

   if( !strcmp( arg2, "number2" ) )
   {
      STRFREE( clan->number2 );
      clan->number2 = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_clan( clan );
      return;
   }

   if( !strcmp( arg2, "board" ) )
   {
      clan->board = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_clan( clan );
      return;
   }

   if( !strcmp( arg2, "members" ) )
   {
      clan->members = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_clan( clan );
      return;
   }

   if( !strcmp( arg2, "funds" ) )
   {
      clan->funds = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_clan( clan );
      return;
   }

   if( !strcmp( arg2, "storage" ) )
   {
      clan->storeroom = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_clan( clan );
      return;
   }

   if( !strcmp( arg2, "guard1" ) )
   {
      clan->guard1 = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_clan( clan );
      return;
   }

   if( !strcmp( arg2, "jail" ) )
   {
      clan->jail = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_clan( clan );
      return;
   }

   if( !strcmp( arg2, "guard2" ) )
   {
      clan->guard2 = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_clan( clan );
      return;
   }

   if( !strcmp( arg2, "trooper1" ) )
   {
      clan->trooper1 = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_clan( clan );
      return;
   }

   if( !strcmp( arg2, "trooper2" ) )
   {
      clan->trooper2 = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_clan( clan );
      return;
   }
   if( !strcmp( arg2, "patrol1" ) )
   {
      clan->patrol1 = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_clan( clan );
      return;
   }

   if( !strcmp( arg2, "patrol2" ) )
   {
      clan->patrol2 = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_clan( clan );
      return;
   }

   if( get_trust( ch ) < LEVEL_SUB_IMPLEM )
   {
      do_setclan( ch, "" );
      return;
   }

   if( !strcmp( arg2, "type" ) )
   {
      if( clan->mainclan )
      {
         UNLINK( clan, clan->mainclan->first_subclan, clan->mainclan->last_subclan, next_subclan, prev_subclan );
         clan->mainclan = NULL;
      }
      if( !str_cmp( argument, "crime" ) )
         clan->clan_type = CLAN_CRIME;
      else if( !str_cmp( argument, "crime family" ) )
         clan->clan_type = CLAN_CRIME;
      else if( !str_cmp( argument, "guild" ) )
         clan->clan_type = CLAN_GUILD;
      else
         clan->clan_type = 0;
      send_to_char( "Done.\r\n", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      CLAN_DATA *uclan = NULL;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You can't name a clan nothing.\r\n", ch );
         return;
      }
      if( ( uclan = get_clan( argument ) ) )
      {
         send_to_char( "There is already another clan with that name.\r\n", ch );
         return;
      }
      STRFREE( clan->name );
      clan->name = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "filename" ) )
   {
      char filename[256];

      if( !is_valid_filename( ch, CLAN_DIR, argument ) )
         return;

      snprintf( filename, sizeof( filename ), "%s%s", CLAN_DIR, clan->filename );
      if( !remove( filename ) )
         send_to_char( "Old clan file deleted.\r\n", ch );

      DISPOSE( clan->filename );
      clan->filename = str_dup( argument );
      send_to_char( "Done.\r\n", ch );
      save_clan( clan );
      write_clan_list(  );
      return;
   }

   if( !strcmp( arg2, "desc" ) )
   {
      STRFREE( clan->description );
      clan->description = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_clan( clan );
      return;
   }

   do_setclan( ch, "" );
   return;
}

void do_setplanet( CHAR_DATA * ch, const char *argument )
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   PLANET_DATA *planet;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' )
   {
      send_to_char( "Usage: setplanet <planet> <field> [value]\r\n", ch );
      send_to_char( "\r\nField being one of:\r\n", ch );
      send_to_char( " base_value flags\r\n", ch );
      send_to_char( " name filename starsystem governed_by\r\n", ch );
      return;
   }

   planet = get_planet( arg1 );
   if( !planet )
   {
      send_to_char( "No such planet.\r\n", ch );
      return;
   }

   if( !strcmp( arg2, "name" ) )
   {
      PLANET_DATA *tplanet;
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You must choose a name.\r\n", ch );
         return;
      }
      if( ( tplanet = get_planet( argument ) ) != NULL )
      {
         send_to_char( "A planet with that name already Exists!\r\n", ch );
         return;
      }

      STRFREE( planet->name );
      planet->name = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_planet( planet );
      return;
   }

   if( !strcmp( arg2, "governed_by" ) )
   {
      CLAN_DATA *clan;
      clan = get_clan( argument );
      if( clan )
      {
         planet->governed_by = clan;
         send_to_char( "Done.\r\n", ch );
         save_planet( planet );
      }
      else
         send_to_char( "No such clan.\r\n", ch );
      return;
   }

   if( !strcmp( arg2, "starsystem" ) )
   {
      SPACE_DATA *starsystem;

      if( ( starsystem = planet->starsystem ) != NULL )
         UNLINK( planet, starsystem->first_planet, starsystem->last_planet, next_in_system, prev_in_system );
      if( ( planet->starsystem = starsystem_from_name( argument ) ) )
      {
         starsystem = planet->starsystem;
         LINK( planet, starsystem->first_planet, starsystem->last_planet, next_in_system, prev_in_system );
         send_to_char( "Done.\r\n", ch );
      }
      else
         send_to_char( "No such starsystem.\r\n", ch );
      save_planet( planet );
      return;
   }

   if( !strcmp( arg2, "filename" ) )
   {
      PLANET_DATA *tplanet;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You must choose a file name.\r\n", ch );
         return;
      }
      for( tplanet = first_planet; tplanet; tplanet = tplanet->next )
      {
          if( !str_cmp( tplanet->filename, argument ) )
          {
              send_to_char( "A planet with that filename already exists!\r\n", ch );
              return;
          }
      }

      DISPOSE( planet->filename );
      planet->filename = str_dup( argument );
      send_to_char( "Done.\r\n", ch );
      save_planet( planet );
      write_planet_list(  );
      return;
   }


   if( !strcmp( arg2, "base_value" ) )
   {
      planet->base_value = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_planet( planet );
      return;
   }

   if( !strcmp( arg2, "flags" ) )
   {
      char farg[MAX_INPUT_LENGTH];

      argument = one_argument( argument, farg );

      if( farg[0] == '\0' )
      {
         send_to_char( "Possible flags: nocapture\r\n", ch );
         return;
      }

      for( ; farg[0] != '\0'; argument = one_argument( argument, farg ) )
      {
         if( !str_cmp( farg, "nocapture" ) )
            TOGGLE_BIT( planet->flags, PLANET_NOCAPTURE );
         else
            ch_printf( ch, "No such flag: %s\r\n", farg );
      }
      send_to_char( "Done.\r\n", ch );
      save_planet( planet );
      return;
   }

   do_setplanet( ch, "" );
   return;
}

void do_showclan( CHAR_DATA * ch, const char *argument )
{
   CLAN_DATA *clan;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      send_to_char( "Usage: showclan <clan>\r\n", ch );
      return;
   }

   clan = get_clan( argument );
   if( !clan )
   {
      send_to_char( "No such clan.\r\n", ch );
      return;
   }

   ch_printf( ch, "%s      : %s\r\nFilename: %s\r\n",
              clan->clan_type == CLAN_CRIME ? "Crime Family " :
              clan->clan_type == CLAN_GUILD ? "Guild " : "Organization ", clan->name, clan->filename );
   ch_printf( ch, "Description: %s\r\nLeader: %s\r\n", clan->description, clan->leader );
   ch_printf( ch, "Number1: %s\r\nNumber2: %s\r\nPKills: %6d    PDeaths: %6d\r\n",
              clan->number1, clan->number2, clan->pkills, clan->pdeaths );
   ch_printf( ch, "MKills: %6d    MDeaths: %6d\r\n", clan->mkills, clan->mdeaths );
   ch_printf( ch, "Type: %d\r\n", clan->clan_type );
   ch_printf( ch, "Members: %3d\r\n", clan->members );
   ch_printf( ch, "Board: %5d   Jail: %5d\r\n", clan->board, clan->jail );
   ch_printf( ch, "Guard1: %5d  Guard2: %5d\r\n", clan->guard1, clan->guard2 );
   ch_printf( ch, "Patrol1: %5d  Patrol2: %5d\r\n", clan->patrol1, clan->patrol2 );
   ch_printf( ch, "Trooper1: %5d  Trooper2: %5d\r\n", clan->trooper1, clan->trooper2 );
   ch_printf( ch, "Funds: %ld\r\n", clan->funds );
   return;
}

void do_showplanet( CHAR_DATA * ch, const char *argument )
{
   PLANET_DATA *planet;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      send_to_char( "Usage: showplanet <planet>\r\n", ch );
      return;
   }

   planet = get_planet( argument );
   if( !planet )
   {
      send_to_char( "No such planet.\r\n", ch );
      return;
   }

   ch_printf( ch, "%s\r\nFilename: %s\r\n", planet->name, planet->filename );
   return;
}

void do_makeclan( CHAR_DATA * ch, const char *argument )
{
   CLAN_DATA *clan;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: makeclan <clan name>\r\n", ch );
      return;
   }

/*  Otherwise it would be possible to create multiple clans
    with the same name. Bad thing... */

   set_char_color( AT_PLAIN, ch );
   clan = get_clan( argument );
   if( clan )
   {
      send_to_char( "There is already a clan with that name.\r\n", ch );
      return;
   }

   CREATE( clan, CLAN_DATA, 1 );
   LINK( clan, first_clan, last_clan, next, prev );
   clan->next_subclan = NULL;
   clan->prev_subclan = NULL;
   clan->last_subclan = NULL;
   clan->first_subclan = NULL;
   clan->mainclan = NULL;
   clan->name = STRALLOC( argument );
   clan->description = STRALLOC( "" );
   clan->leader = STRALLOC( "" );
   clan->number1 = STRALLOC( "" );
   clan->number2 = STRALLOC( "" );
   clan->tmpstr = STRALLOC( "" );
}

void do_makeplanet( CHAR_DATA * ch, const char *argument )
{
   PLANET_DATA *planet;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: makeplanet <planet name>\r\n", ch );
      return;
   }

/*  Otherwise it would be possible to create multiple clans
    with the same name. Bad thing... */

   set_char_color( AT_PLAIN, ch );
   planet = get_planet( argument );
   if( planet )
   {
      send_to_char( "There is already a planet with that name.\r\n", ch );
      return;
   }

   CREATE( planet, PLANET_DATA, 1 );
   LINK( planet, first_planet, last_planet, next, prev );
   planet->governed_by = NULL;
   planet->next_in_system = NULL;
   planet->prev_in_system = NULL;
   planet->starsystem = NULL;
   planet->first_area = NULL;
   planet->last_area = NULL;
   planet->first_guard = NULL;
   planet->last_guard = NULL;
   planet->name = STRALLOC( argument );
   planet->flags = 0;
}

void do_clans( CHAR_DATA * ch, const char *argument )
{
   CLAN_DATA *clan;
   PLANET_DATA *planet;
   int count = 0;
   int pCount = 0;
   int support;
   long revenue;

   for( clan = first_clan; clan; clan = clan->next )
   {
      if( clan->clan_type == CLAN_CRIME || clan->clan_type == CLAN_GUILD || clan->clan_type == CLAN_SUBCLAN )
         continue;

      pCount = 0;
      support = 0;
      revenue = 0;

      for( planet = first_planet; planet; planet = planet->next )
         if( clan == planet->governed_by )
         {
	   support += ( int ) planet->pop_support;
            pCount++;
            revenue += get_taxes( planet );
         }

      if( pCount > 1 )
         support /= pCount;

      ch_printf( ch, "&WOrganization: &Y%s\r\n", clan->name );
      ch_printf( ch, "  &WPlanets: &O%-2d       &WAvg Pop Support: &O%-3d&W    &WRevenue: &O%-10ld\r\n",
                 pCount, support, revenue );
      ch_printf( ch, "  &WDivision             Leaders             Spacecraft/Vehicles/Members/Funds\r\n" );
      ch_printf( ch, "  &O%-20s %-10s %-10s %-10s %-2d %-2d %-3d %ld\r\n",
                 "Main", clan->leader, clan->number1, clan->number2, clan->spacecraft, clan->vehicles, clan->members,
                 clan->funds );
      if( clan->first_subclan )
      {
         CLAN_DATA *subclan;

         for( subclan = clan->first_subclan; subclan; subclan = subclan->next_subclan )
            ch_printf( ch, "  &O%-20s %-10s %-10s %-10s %-2d %-2d %-3d %ld\r\n",
                       subclan->name, subclan->leader, subclan->number1, subclan->number2, subclan->spacecraft,
                       subclan->vehicles, subclan->members, subclan->funds );
      }
      ch_printf( ch, "\r\n" );
      count++;
   }

   ch_printf( ch, "&WAutonomous Groups      Leaders             Spacecraft/Vehicles/Members/Funds\r\n" );
   for( clan = first_clan; clan; clan = clan->next )
   {
      if( clan->clan_type != CLAN_CRIME && clan->clan_type != CLAN_GUILD )
         continue;

      ch_printf( ch, "&Y%-22s &O%-10s %-10s %-10s %-2d %-2d %-3d %ld\r\n",
                 clan->name, clan->leader, clan->number1, clan->number2, clan->spacecraft, clan->vehicles, clan->members,
                 clan->funds );
      count++;
   }

   if( !count )
   {
      set_char_color( AT_BLOOD, ch );
      send_to_char( "There are no organizations currently formed.\r\n", ch );
   }

   set_char_color( AT_WHITE, ch );
   send_to_char( "\r\nSee also: Planets and Senate\r\n", ch );

}

void do_planets( CHAR_DATA * ch, const char *argument )
{
   PLANET_DATA *planet;
   int count = 0;
   AREA_DATA *area;

   set_char_color( AT_WHITE, ch );
   for( planet = first_planet; planet; planet = planet->next )
   {
      ch_printf( ch, "&WPlanet: &G%-15s   &WGoverned By: &G%s %s\r\n",
                 planet->name,
                 planet->governed_by ? planet->governed_by->name : "",
                 IS_SET( planet->flags, PLANET_NOCAPTURE ) ? "(permanent)" : "" );
      ch_printf( ch, "&WValue: &G%-10ld&W/&G%-10d   ", get_taxes( planet ), planet->base_value );
      ch_printf( ch, "&WPopulation: &G%-5d   &W Pop Support: &G%.1f\r\n", planet->population, planet->pop_support );
      if( IS_IMMORTAL( ch ) )
      {
         ch_printf( ch, "&WAreas: &G" );
         for( area = planet->first_area; area; area = area->next_on_planet )
            ch_printf( ch, "%s,  ", area->filename );
         ch_printf( ch, "\r\n" );
      }
      ch_printf( ch, "\r\n" );

      count++;
   }

   if( !count )
   {
      set_char_color( AT_BLOOD, ch );
      send_to_char( "There are no planets currently formed.\r\n", ch );
   }

}

void do_orders( CHAR_DATA * ch, const char *argument )
{
}

void do_guilds( CHAR_DATA * ch, const char *argument )
{
}

void do_shove( CHAR_DATA * ch, const char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   int exit_dir;
   EXIT_DATA *pexit;
   CHAR_DATA *victim;
   bool nogo;
   ROOM_INDEX_DATA *to_room;
   int schance;

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );


   if( arg[0] == '\0' )
   {
      send_to_char( "Shove whom?\r\n", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "You shove yourself around, to no avail.\r\n", ch );
      return;
   }

   if( ( victim->position ) != POS_STANDING )
   {
      act( AT_PLAIN, "$N isn't standing up.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( arg2[0] == '\0' )
   {
      send_to_char( "Shove them in which direction?\r\n", ch );
      return;
   }

   exit_dir = get_dir( arg2 );
   if( IS_SET( victim->in_room->room_flags, ROOM_SAFE ) && get_timer( victim, TIMER_SHOVEDRAG ) <= 0 )
   {
      send_to_char( "That character cannot be shoved right now.\r\n", ch );
      return;
   }
   victim->position = POS_SHOVE;
   nogo = FALSE;
   if( ( pexit = get_exit( ch->in_room, exit_dir ) ) == NULL )
      nogo = TRUE;
   else
      if( IS_SET( pexit->exit_info, EX_CLOSED )
          && ( !IS_AFFECTED( victim, AFF_PASS_DOOR ) || IS_SET( pexit->exit_info, EX_NOPASSDOOR ) ) )
      nogo = TRUE;
   if( nogo )
   {
      send_to_char( "There's no exit in that direction.\r\n", ch );
      victim->position = POS_STANDING;
      return;
   }
   to_room = pexit->to_room;

   if( IS_NPC( victim ) )
   {
      send_to_char( "You can only shove player characters.\r\n", ch );
      return;
   }

   if( ch->in_room->area != to_room->area && !in_hard_range( victim, to_room->area ) )
   {
      send_to_char( "That character cannot enter that area.\r\n", ch );
      victim->position = POS_STANDING;
      return;
   }

   schance = 50;

/* Add 3 points to chance for every str point above 15, subtract for 
below 15 */
   schance += ( ( get_curr_str( ch ) - 15 ) * 3 );

   schance += ( ch->top_level - victim->top_level );

/* Debugging purposes - show percentage for testing */

/* sprintf(buf, "Shove percentage of %s = %d", ch->name, chance);
act( AT_ACTION, buf, ch, NULL, NULL, TO_ROOM );
*/

   if( schance < number_percent(  ) )
   {
      send_to_char( "You failed.\r\n", ch );
      victim->position = POS_STANDING;
      return;
   }
   act( AT_ACTION, "You shove $M.", ch, NULL, victim, TO_CHAR );
   act( AT_ACTION, "$n shoves you.", ch, NULL, victim, TO_VICT );
   move_char( victim, get_exit( ch->in_room, exit_dir ), 0 );
   if( !char_died( victim ) )
      victim->position = POS_STANDING;
   WAIT_STATE( ch, 12 );
   /*
    * Remove protection from shove/drag if char shoves -- Blodkai 
    */
   if( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) && get_timer( ch, TIMER_SHOVEDRAG ) <= 0 )
      add_timer( ch, TIMER_SHOVEDRAG, 10, NULL, 0 );
}

void do_drag( CHAR_DATA * ch, const char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   int exit_dir;
   CHAR_DATA *victim;
   EXIT_DATA *pexit;
   ROOM_INDEX_DATA *to_room;
   bool nogo;
   int schance;

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( arg[0] == '\0' )
   {
      send_to_char( "Drag whom?\r\n", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "You take yourself by the scruff of your neck, but go nowhere.\r\n", ch );
      return;
   }

   if( IS_NPC( victim ) )
   {
      send_to_char( "You can only drag player characters.\r\n", ch );
      return;
   }

   if( victim->fighting )
   {
      send_to_char( "You try, but can't get close enough.\r\n", ch );
      return;
   }

   if( arg2[0] == '\0' )
   {
      send_to_char( "Drag them in which direction?\r\n", ch );
      return;
   }

   exit_dir = get_dir( arg2 );

   if( IS_SET( victim->in_room->room_flags, ROOM_SAFE ) && get_timer( victim, TIMER_SHOVEDRAG ) <= 0 )
   {
      send_to_char( "That character cannot be dragged right now.\r\n", ch );
      return;
   }

   nogo = FALSE;
   if( ( pexit = get_exit( ch->in_room, exit_dir ) ) == NULL )
      nogo = TRUE;
   else
      if( IS_SET( pexit->exit_info, EX_CLOSED )
          && ( !IS_AFFECTED( victim, AFF_PASS_DOOR ) || IS_SET( pexit->exit_info, EX_NOPASSDOOR ) ) )
      nogo = TRUE;
   if( nogo )
   {
      send_to_char( "There's no exit in that direction.\r\n", ch );
      return;
   }

   to_room = pexit->to_room;

   if( ch->in_room->area != to_room->area && !in_hard_range( victim, to_room->area ) )
   {
      send_to_char( "That character cannot enter that area.\r\n", ch );
      victim->position = POS_STANDING;
      return;
   }

   schance = 50;


/*
sprintf(buf, "Drag percentage of %s = %d", ch->name, chance);
act( AT_ACTION, buf, ch, NULL, NULL, TO_ROOM );
*/
   if( schance < number_percent(  ) )
   {
      send_to_char( "You failed.\r\n", ch );
      victim->position = POS_STANDING;
      return;
   }
   if( victim->position < POS_STANDING )
   {
      short temp;

      temp = victim->position;
      victim->position = POS_DRAG;
      act( AT_ACTION, "You drag $M into the next room.", ch, NULL, victim, TO_CHAR );
      act( AT_ACTION, "$n grabs your hair and drags you.", ch, NULL, victim, TO_VICT );
      move_char( victim, get_exit( ch->in_room, exit_dir ), 0 );
      if( !char_died( victim ) )
         victim->position = temp;
/* Move ch to the room too.. they are doing dragging - Scryn */
      move_char( ch, get_exit( ch->in_room, exit_dir ), 0 );
      WAIT_STATE( ch, 12 );
      return;
   }
   send_to_char( "You cannot do that to someone who is standing.\r\n", ch );
   return;
}

void do_enlist( CHAR_DATA * ch, const char *argument )
{

   CLAN_DATA *clan;

   if( IS_NPC( ch ) || !ch->pcdata )
   {
      send_to_char( "You can't do that.\r\n", ch );
      return;
   }

   if( ch->pcdata->clan )
   {
      ch_printf( ch, "You will have to resign from %s before you can join a new organization.\r\n", ch->pcdata->clan->name );
      return;
   }

   if( IS_SET( ch->in_room->room_flags, ROOM_R_RECRUIT ) )
      clan = get_clan( "The New Republic" );
   else if( IS_SET( ch->in_room->room_flags, ROOM_E_RECRUIT ) )
      clan = get_clan( "The Empire" );
   else
   {
      send_to_char( "You don't seem to be in a recruitment office.\r\n", ch );
      return;
   }

   if( !clan )
   {
      send_to_char( "They don't seem to be recruiting right now.\r\n", ch );
      return;
   }

   SET_BIT( ch->speaks, LANG_CLAN );
   ++clan->members;

   STRFREE( ch->pcdata->clan_name );
   ch->pcdata->clan_name = QUICKLINK( clan->name );
   ch->pcdata->clan = clan;
   ch_printf( ch, "Welcome to %s.\r\n", clan->name );

   save_clan( clan );
   return;

}

void do_resign( CHAR_DATA * ch, const char *argument )
{

   CLAN_DATA *clan;
   long lose_exp;
   char buf[MAX_STRING_LENGTH];

   if( IS_NPC( ch ) || !ch->pcdata )
   {
      send_to_char( "You can't do that.\r\n", ch );
      return;
   }

   clan = ch->pcdata->clan;

   if( clan == NULL )
   {
      send_to_char( "You have to join an organization before you can quit it.\r\n", ch );
      return;
   }

   if( !str_cmp( ch->name, ch->pcdata->clan->leader ) )
   {
      ch_printf( ch, "You can't resign from %s ... you are the leader!\r\n", clan->name );
      return;
   }

   if( ch->speaking & LANG_CLAN )
      ch->speaking = LANG_COMMON;
   REMOVE_BIT( ch->speaks, LANG_CLAN );
   --clan->members;
   if( !str_cmp( ch->name, ch->pcdata->clan->number1 ) )
   {
      STRFREE( ch->pcdata->clan->number1 );
      ch->pcdata->clan->number1 = STRALLOC( "" );
   }
   if( !str_cmp( ch->name, ch->pcdata->clan->number2 ) )
   {
      STRFREE( ch->pcdata->clan->number2 );
      ch->pcdata->clan->number2 = STRALLOC( "" );
   }
   ch->pcdata->clan = NULL;
   STRFREE( ch->pcdata->clan_name );
   ch->pcdata->clan_name = STRALLOC( "" );
   act( AT_MAGIC, "You resign your position in $t", ch, clan->name, NULL, TO_CHAR );
   sprintf( buf, "%s has quit %s!", ch->name, clan->name );
   echo_to_all( AT_MAGIC, buf, ECHOTAR_ALL );

   lose_exp = UMAX( ch->experience[DIPLOMACY_ABILITY] - exp_level( ch->skill_level[DIPLOMACY_ABILITY] ), 0 );
   ch_printf( ch, "You lose %ld diplomacy experience.\r\n", lose_exp );
   ch->experience[DIPLOMACY_ABILITY] -= lose_exp;

   DISPOSE( ch->pcdata->bestowments );
   ch->pcdata->bestowments = str_dup( "" );

   save_char_obj( ch ); /* clan gets saved when pfile is saved */

   return;

}

void do_clan_withdraw( CHAR_DATA * ch, const char *argument )
{
   CLAN_DATA *clan;
   long amount;

   if( IS_NPC( ch ) || !ch->pcdata->clan )
   {
      send_to_char( "You don't seem to belong to an organization to withdraw funds from...\r\n", ch );
      return;
   }

   if( !ch->in_room || !IS_SET( ch->in_room->room_flags, ROOM_BANK ) )
   {
      send_to_char( "You must be in a bank to do that!\r\n", ch );
      return;
   }

   if( ( ch->pcdata && ch->pcdata->bestowments
         && is_name( "withdraw", ch->pcdata->bestowments ) ) || !str_cmp( ch->name, ch->pcdata->clan->leader ) )
      ;
   else
   {
      send_to_char( "&RYour organization hasn't seen fit to bestow you with that ability.", ch );
      return;
   }

   clan = ch->pcdata->clan;

   amount = atoi( argument );

   if( !amount )
   {
      send_to_char( "How much would you like to withdraw?\r\n", ch );
      return;
   }

   if( amount > clan->funds )
   {
      ch_printf( ch, "%s doesn't have that much!\r\n", clan->name );
      return;
   }

   if( amount < 0 )
   {
      ch_printf( ch, "Nice try...\r\n" );
      return;
   }

   ch_printf( ch, "You withdraw %ld credits from %s's funds.\r\n", amount, clan->name );

   clan->funds -= amount;
   ch->gold += amount;
   save_char_obj( ch );
}


void do_clan_donate( CHAR_DATA * ch, const char *argument )
{
   CLAN_DATA *clan;
   long amount;

   if( IS_NPC( ch ) || !ch->pcdata->clan )
   {
      send_to_char( "You don't seem to belong to an organization to donate to...\r\n", ch );
      return;
   }

   if( !ch->in_room || !IS_SET( ch->in_room->room_flags, ROOM_BANK ) )
   {
      send_to_char( "You must be in a bank to do that!\r\n", ch );
      return;
   }

   clan = ch->pcdata->clan;

   amount = atoi( argument );

   if( !amount )
   {
      send_to_char( "How much would you like to donate?\r\n", ch );
      return;
   }

   if( amount < 0 )
   {
      ch_printf( ch, "Nice try...\r\n" );
      return;
   }

   if( amount > ch->gold )
   {
      send_to_char( "You don't have that much!\r\n", ch );
      return;
   }

   ch_printf( ch, "You donate %ld credits to %s's funds.\r\n", amount, clan->name );

   clan->funds += amount;
   ch->gold -= amount;
   save_char_obj( ch );
}

void do_newclan( CHAR_DATA * ch, const char *argument )
{
   send_to_char( "This command is being recycled to conserve thought.\r\n", ch );
   return;
}

void do_appoint( CHAR_DATA * ch, const char *argument )
{
   char arg[MAX_STRING_LENGTH];

   argument = one_argument( argument, arg );

   if( IS_NPC( ch ) || !ch->pcdata )
      return;

   if( !ch->pcdata->clan )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( str_cmp( ch->name, ch->pcdata->clan->leader ) )
   {
      send_to_char( "Only your leader can do that!\r\n", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      send_to_char( "Useage: appoint <name> < first | second >\r\n", ch );
      return;
   }

   if( !str_cmp( argument, "first" ) )
   {
      if( ch->pcdata->clan->number1 && str_cmp( ch->pcdata->clan->number1, "" ) )
      {
         send_to_char( "You already have someone in that position ... demote them first.\r\n", ch );
         return;
      }

      STRFREE( ch->pcdata->clan->number1 );
      ch->pcdata->clan->number1 = STRALLOC( arg );
   }
   else if( !str_cmp( argument, "second" ) )
   {
      if( ch->pcdata->clan->number2 && str_cmp( ch->pcdata->clan->number2, "" ) )
      {
         send_to_char( "You already have someone in that position ... demote them first.\r\n", ch );
         return;
      }

      STRFREE( ch->pcdata->clan->number2 );
      ch->pcdata->clan->number2 = STRALLOC( arg );
   }
   else
      do_appoint( ch, "" );
   save_clan( ch->pcdata->clan );

}

void do_demote( CHAR_DATA * ch, const char *argument )
{

   if( IS_NPC( ch ) || !ch->pcdata )
      return;

   if( !ch->pcdata->clan )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( str_cmp( ch->name, ch->pcdata->clan->leader ) )
   {
      send_to_char( "Only your leader can do that!\r\n", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      send_to_char( "Demote who?\r\n", ch );
      return;
   }

   if( !str_cmp( argument, ch->pcdata->clan->number1 ) )
   {
      send_to_char( "Player Demoted!", ch );

      STRFREE( ch->pcdata->clan->number1 );
      ch->pcdata->clan->number1 = STRALLOC( "" );
   }
   else if( !str_cmp( argument, ch->pcdata->clan->number2 ) )
   {
      send_to_char( "Player Demoted!", ch );

      STRFREE( ch->pcdata->clan->number2 );
      ch->pcdata->clan->number2 = STRALLOC( "" );
   }
   else
   {
      send_to_char( "They seem to have been demoted already.\r\n", ch );
      return;
   }
   save_clan( ch->pcdata->clan );

}

void do_capture( CHAR_DATA * ch, const char *argument )
{
   CLAN_DATA *clan;
   PLANET_DATA *planet;
   PLANET_DATA *cPlanet;
   float support = 0.0;
   int pCount = 0;
   char buf[MAX_STRING_LENGTH];

   if( !ch->in_room || !ch->in_room->area )
      return;

   if( IS_NPC( ch ) || !ch->pcdata )
   {
      send_to_char( "huh?\r\n", ch );
      return;
   }

   if( !ch->pcdata->clan )
   {
      send_to_char( "You need to be a member of an organization to do that!\r\n", ch );
      return;
   }

   if( ch->pcdata->clan->mainclan )
      clan = ch->pcdata->clan->mainclan;
   else
      clan = ch->pcdata->clan;

   if( clan->clan_type == CLAN_CRIME )
   {
      send_to_char( "Crime fimilies aren't in the business of controlling worlds.\r\n", ch );
      return;
   }

   if( clan->clan_type == CLAN_GUILD )
   {
      send_to_char( "Your organization serves a much greater purpose.\r\n", ch );
      return;
   }

   if( ( planet = ch->in_room->area->planet ) == NULL )
   {
      send_to_char( "You must be on a planet to capture it.\r\n", ch );
      return;
   }

   if( clan == planet->governed_by )
   {
      send_to_char( "Your organization already controls this planet.\r\n", ch );
      return;
   }

   if( planet->starsystem )
   {
      SHIP_DATA *ship;
      CLAN_DATA *sClan;

      for( ship = planet->starsystem->first_ship; ship; ship = ship->next_in_starsystem )
      {
         sClan = get_clan( ship->owner );
         if( !sClan )
            continue;
         if( sClan->mainclan )
            sClan = sClan->mainclan;
         if( sClan == planet->governed_by )
         {
            send_to_char( "A planet cannot be captured while protected by orbiting spacecraft.\r\n", ch );
            return;
         }
      }
   }

   if( IS_SET( planet->flags, PLANET_NOCAPTURE ) )
   {
      send_to_char( "This planet cannot be captured.\r\n", ch );
      return;
   }

   if( planet->pop_support > 0 )
   {
      send_to_char( "The population is not in favour of changing leaders right now.\r\n", ch );
      return;
   }

   for( cPlanet = first_planet; cPlanet; cPlanet = cPlanet->next )
      if( clan == cPlanet->governed_by )
      {
         pCount++;
         support += cPlanet->pop_support;
      }

   if( support < 0 )
   {
      send_to_char
         ( "There is not enough popular support for your organization!\r\nTry improving loyalty on the planets that you already control.\r\n",
           ch );
      return;
   }

   planet->governed_by = clan;
   planet->pop_support = 50;

   sprintf( buf, "%s has been captured by %s!", planet->name, clan->name );
   echo_to_all( AT_RED, buf, 0 );

   save_planet( planet );

   return;
}

void do_empower( CHAR_DATA * ch, const char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   CLAN_DATA *clan;
   char buf[MAX_STRING_LENGTH];

   if( IS_NPC( ch ) || !ch->pcdata->clan )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   clan = ch->pcdata->clan;

   if( ( ch->pcdata && ch->pcdata->bestowments
         && is_name( "withdraw", ch->pcdata->bestowments ) ) || !str_cmp( ch->name, clan->leader ) )
      ;
   else
   {
      send_to_char( "You clan hasn't seen fit to bestow that ability to you!\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( arg[0] == '\0' )
   {
      send_to_char( "Empower whom to do what?\r\n", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "That player is not here.\r\n", ch );
      return;
   }

   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "Nice try.\r\n", ch );
      return;
   }

   if( victim->pcdata->clan != ch->pcdata->clan )
   {
      send_to_char( "This player does not belong to your clan!\r\n", ch );
      return;
   }

   if( !victim->pcdata->bestowments )
      victim->pcdata->bestowments = str_dup( "" );

   if( arg2[0] == '\0' || !str_cmp( arg2, "list" ) )
   {
      ch_printf( ch, "Current bestowed commands on %s: %s.\r\n", victim->name, victim->pcdata->bestowments );
      return;
   }

   if( str_cmp( ch->name, clan->leader ) && !str_cmp( ch->name, clan->number1 ) )
   {
      if( !is_name( arg2, ch->pcdata->bestowments ) )
      {
         send_to_char( "&RI don't think you're even allowed to do that.&W\r\n", ch );
         return;
      }
   }

   if( !str_cmp( arg2, "none" ) )
   {
      DISPOSE( victim->pcdata->bestowments );
      victim->pcdata->bestowments = str_dup( "" );
      ch_printf( ch, "Bestowments removed from %s.\r\n", victim->name );
      ch_printf( victim, "%s has removed your bestowed clan abilities.\r\n", ch->name );
      return;
   }
   else if( !str_cmp( arg2, "pilot" ) )
   {
      sprintf( buf, "%s %s", victim->pcdata->bestowments, arg2 );
      DISPOSE( victim->pcdata->bestowments );
      victim->pcdata->bestowments = str_dup( buf );
      ch_printf( victim, "%s has given you permission to fly clan ships.\r\n", ch->name );
      send_to_char( "Ok, they now have the ability to fly clan ships.\r\n", ch );
   }
   else if( !str_cmp( arg2, "withdraw" ) )
   {
      sprintf( buf, "%s %s", victim->pcdata->bestowments, arg2 );
      DISPOSE( victim->pcdata->bestowments );
      victim->pcdata->bestowments = str_dup( buf );
      ch_printf( victim, "%s has given you permission to withdraw clan funds.\r\n", ch->name );
      send_to_char( "Ok, they now have the ablitity to withdraw clan funds.\r\n", ch );
   }
   else if( !str_cmp( arg2, "clanbuyship" ) )
   {
      sprintf( buf, "%s %s", victim->pcdata->bestowments, arg2 );
      DISPOSE( victim->pcdata->bestowments );
      victim->pcdata->bestowments = str_dup( buf );
      ch_printf( victim, "%s has given you permission to buy clan ships.\r\n", ch->name );
      send_to_char( "Ok, they now have the ablitity to use clanbuyship.\r\n", ch );
   }
   else if( !str_cmp( arg2, "induct" ) )
   {
      sprintf( buf, "%s %s", victim->pcdata->bestowments, arg2 );
      DISPOSE( victim->pcdata->bestowments );
      victim->pcdata->bestowments = str_dup( buf );
      ch_printf( victim, "%s has given you permission to induct new members.\r\n", ch->name );
      send_to_char( "Ok, they now have the ablitity to induct new members.\r\n", ch );
   }
   else
   {
      send_to_char( "Currently you may empower members with only the following:\r\n", ch );
      send_to_char( "\r\npilot:       ability to fly clan ships\r\n", ch );
      send_to_char( "withdraw:    ability to withdraw clan funds\r\n", ch );
      send_to_char( "clanbuyship: ability to buy clan ships\r\n", ch );
      send_to_char( "induct:      ability to induct new members\r\n", ch );
      send_to_char( "none:        removes bestowed abilities\r\n", ch );
   }

   save_char_obj( victim );   /* clan gets saved when pfile is saved */
   return;
}

void save_senate(  )
{
/*
    BOUNTY_DATA *tbounty;
    FILE *fpout;
    char filename[256];
    
    sprintf( filename, "%s%s", SYSTEM_DIR, BOUNTY_LIST );
    fpout = fopen( filename, "w" );
    if ( !fpout )
    {
         bug( "FATAL: cannot open bounty.lst for writing!\r\n", 0 );
         return;
    }
    for ( tbounty = first_bounty; tbounty; tbounty = tbounty->next )
    {
        fprintf( fpout, "%s\n", tbounty->target );
        fprintf( fpout, "%ld\n", tbounty->amount );
    }
    fprintf( fpout, "$\n" );
    fclose( fpout );
*/
}

void load_senate(  )
{
   first_senator = NULL;
   last_senator = NULL;
/*
    FILE *fpList;
    char *target;
    char bountylist[256];
    BOUNTY_DATA *bounty;
    long int  amount;
     
    first_bounty = NULL;
    last_bounty	= NULL;

    first_disintigration = NULL;
    last_disintigration	= NULL;

    log_string( "Loading disintigrations..." );

    sprintf( bountylist, "%s%s", SYSTEM_DIR, DISINTIGRATION_LIST );
    if ( ( fpList = fopen( bountylist, "r" ) ) == NULL )
    {
	perror( bountylist );
	exit( 1 );
    }

    for ( ; ; )
    {
        target = feof( fpList ) ? "$" : fread_word( fpList );
        if ( target[0] == '$' )
        break;                                  
	CREATE( bounty, BOUNTY_DATA, 1 );
        LINK( bounty, first_disintigration, last_disintigration, next, prev );
	bounty->target = STRALLOC(target);
	amount = fread_number( fpList );
	bounty->amount = amount;
    }
    fclose( fpList );
    log_string(" Done bounties " );
    return;
*/
}

void do_senate( CHAR_DATA * ch, const char *argument )
{
/*
    GOV_DATA *gov;
    int count = 0;
    
    set_char_color( AT_WHITE, ch );
    send_to_char( "\r\nGoverning Area                 Controlled By             Value\r\n", ch );
    for ( gov = first_gov; gov; gov = gov->next )
    {
        set_char_color( AT_YELLOW, ch );
        ch_printf( ch, "%-30s %-25s %-15ld\r\n", gov->name, gov->controlled_by , gov->value );
        count++;
    }

    if ( !count )
    {
        set_char_color( AT_GREY, ch );
        send_to_char( "There are no governments to capture at this time.\r\n", ch );
	return;
    }
*/
}

void do_addsenator( CHAR_DATA * ch, const char *argument )
{
/*
    GOVE_DATA *gov;
    
    CREATE( gov, GOV_DATA, 1 );
    LINK( gov, first_gov, last_gov, next, prev );

    gov->name		= STRALLOC( argument );
    gov->value          = atoi( arg2 );
    gov->vnum           = object;
    gov->controlled_by  = STRALLOC( "" );
        
    ch_printf( ch, "OK, making %s.\r\n", argument );
    save_govs();
*/
}

void do_remsenator( CHAR_DATA * ch, const char *argument )
{
/*
	UNLINK( bounty, first_bounty, last_bounty, next, prev );
	STRFREE( bounty->target );
	DISPOSE( bounty );
	
	save_bounties();
*/
}

long get_taxes( PLANET_DATA * planet )
{
   long gain;

   gain = planet->base_value;
   gain += ( long )( planet->base_value * planet->pop_support / 100 );
   gain += UMAX( 0, ( int )( planet->pop_support / 10 * planet->population ) );

   return gain;
}

/*
    (link)->prev		= (insert)->prev;		
    if ( !(insert)->prev )					
      (first)			= (link);			
    else							
      (insert)->prev->next	= (link);			
    (insert)->prev		= (link);			
    (link)->next		= (insert);			
*/
