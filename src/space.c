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
*		                Space Module    			   *   
****************************************************************************/

#include <math.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mud.h"

SHIP_DATA *first_ship;
SHIP_DATA *last_ship;

MISSILE_DATA *first_missile;
MISSILE_DATA *last_missile;

SPACE_DATA *first_starsystem;
SPACE_DATA *last_starsystem;

int bus_pos = 0;
int bus_planet = 0;
int bus2_planet = 4;
int turbocar_stop = 0;
int corus_shuttle = 0;

#define MAX_STATION    10
#define MAX_BUS_STOP 10

#define STOP_PLANET     202
#define STOP_SHIPYARD   32015

#define SENATEPAD       10196
#define OUTERPAD        10195

int const station_vnum[MAX_STATION] = {
   215, 216, 217, 218, 219, 220, 221, 222, 223, 224
};

const char *const station_name[MAX_STATION] = {
   "Menari Spaceport", "Skydome Botanical Gardens", "Grand Towers",
   "Grandis Mon Theater", "Palace Station", "Great Galactic Museum",
   "College Station", "Holographic Zoo of Extinct Animals",
   "Dometown Station ", "Monument Plaza"
};

int const bus_vnum[MAX_BUS_STOP] = {
   201, 21100, 29001, 28038, 31872, 1001, 28613, 3060, 28247, 32297
};

const char *const bus_stop[MAX_BUS_STOP + 1] = {
   "Coruscant",
   "Mon Calamari", "Adari", "Gamorr", "Tatooine", "Honoghr",   /* "Ryloth", */
   "Kashyyyk", "Endor", "Byss", "Cloning Facilities", "Coruscant" /* last should always be same as first */
};

/* local routines */
void fread_ship args( ( SHIP_DATA * ship, FILE * fp ) );
bool load_ship_file( const char *shipfile );
void write_ship_list args( ( void ) );
void fread_starsystem args( ( SPACE_DATA * starsystem, FILE * fp ) );
bool load_starsystem( const char *starsystemfile );
void write_starsystem_list args( ( void ) );
void resetship args( ( SHIP_DATA * ship ) );
void landship( SHIP_DATA * ship, const char *arg );
void launchship args( ( SHIP_DATA * ship ) );
bool land_bus args( ( SHIP_DATA * ship, int destination ) );
void launch_bus args( ( SHIP_DATA * ship ) );
void echo_to_room_dnr( int ecolor, ROOM_INDEX_DATA * room, const char *argument );
ch_ret drive_ship( CHAR_DATA * ch, SHIP_DATA * ship, EXIT_DATA * pexit, int fall );
bool autofly( SHIP_DATA * ship );
bool is_facing( SHIP_DATA * ship, SHIP_DATA * target );
void sound_to_ship( SHIP_DATA * ship, char *argument );

/* from comm.c */
bool write_to_descriptor args( ( int desc, char *txt, int length ) );

void echo_to_room_dnr( int ecolor, ROOM_INDEX_DATA * room, const char *argument )
{
   CHAR_DATA *vic;

   if( room == NULL )
      return;

   for( vic = room->first_person; vic; vic = vic->next_in_room )
   {
      set_char_color( ecolor, vic );
      send_to_char( argument, vic );
   }
}


bool land_bus( SHIP_DATA * ship, int destination )
{
   char buf[MAX_STRING_LENGTH];

   if( !ship_to_room( ship, destination ) )
   {
      return FALSE;
   }
   echo_to_ship( AT_YELLOW, ship, "You feel a slight thud as the ship sets down on the ground." );
   ship->location = destination;
   ship->lastdoc = ship->location;
   ship->shipstate = SHIP_DOCKED;
   if( ship->starsystem )
      ship_from_starsystem( ship, ship->starsystem );
   sprintf( buf, "%s lands on the platform.", ship->name );
   echo_to_room( AT_YELLOW, get_room_index( ship->location ), buf );
   sprintf( buf, "The hatch on %s opens.", ship->name );
   echo_to_room( AT_YELLOW, get_room_index( ship->location ), buf );
   echo_to_room( AT_YELLOW, get_room_index( ship->entrance ), "The hatch opens." );
   ship->hatchopen = TRUE;
   sound_to_room( get_room_index( ship->entrance ), "!!SOUND(door)" );
   sound_to_room( get_room_index( ship->location ), "!!SOUND(door)" );
   return TRUE;
}

void launch_bus( SHIP_DATA * ship )
{
   char buf[MAX_STRING_LENGTH];

   sound_to_room( get_room_index( ship->entrance ), "!!SOUND(door)" );
   sound_to_room( get_room_index( ship->location ), "!!SOUND(door)" );
   sprintf( buf, "The hatch on %s closes and it begins to launch.", ship->name );
   echo_to_room( AT_YELLOW, get_room_index( ship->location ), buf );
   echo_to_room( AT_YELLOW, get_room_index( ship->entrance ), "The hatch slides shut." );
   ship->hatchopen = FALSE;
   extract_ship( ship );
   echo_to_ship( AT_YELLOW, ship, "The ship begins to launch." );
   ship->location = 0;
   ship->shipstate = SHIP_READY;
}

void update_traffic(  )
{
   SHIP_DATA *shuttle, *senate;
   SHIP_DATA *turbocar;
   char buf[MAX_STRING_LENGTH];

   shuttle = ship_from_cockpit( ROOM_CORUSCANT_SHUTTLE );
   senate = ship_from_cockpit( ROOM_SENATE_SHUTTLE );
   if( senate != NULL && shuttle != NULL )
   {
      switch ( corus_shuttle )
      {
         default:
            corus_shuttle++;
            break;

         case 0:
            land_bus( shuttle, STOP_PLANET );
            land_bus( senate, SENATEPAD );
            corus_shuttle++;
            echo_to_ship( AT_CYAN, shuttle, "Welcome to Menari Spaceport." );
            echo_to_ship( AT_CYAN, senate, "Welcome to The Senate Halls." );
            break;

         case 4:
            launch_bus( shuttle );
            launch_bus( senate );
            corus_shuttle++;
            break;

         case 5:
            land_bus( shuttle, STOP_SHIPYARD );
            land_bus( senate, OUTERPAD );
            echo_to_ship( AT_CYAN, shuttle, "Welcome to Coruscant Shipyard." );
            echo_to_ship( AT_CYAN, senate, "Welcome to The Outer System Landing Area." );
            corus_shuttle++;
            break;

         case 9:
            launch_bus( shuttle );
            launch_bus( senate );
            corus_shuttle++;
            break;

      }

      if( corus_shuttle >= 10 )
         corus_shuttle = 0;
   }

   turbocar = ship_from_cockpit( ROOM_CORUSCANT_TURBOCAR );
   if( turbocar != NULL )
   {
      sprintf( buf, "The turbocar doors close and it speeds out of the station." );
      echo_to_room( AT_YELLOW, get_room_index( turbocar->location ), buf );
      extract_ship( turbocar );
      turbocar->location = 0;
      ship_to_room( turbocar, station_vnum[turbocar_stop] );
      echo_to_ship( AT_YELLOW, turbocar, "The turbocar makes a quick journey to the next station." );
      turbocar->location = station_vnum[turbocar_stop];
      turbocar->lastdoc = turbocar->location;
      turbocar->shipstate = SHIP_DOCKED;
      if( turbocar->starsystem )
         ship_from_starsystem( turbocar, turbocar->starsystem );
      sprintf( buf, "A turbocar pulls into the platform and the doors slide open." );
      echo_to_room( AT_YELLOW, get_room_index( turbocar->location ), buf );
      sprintf( buf, "Welcome to %s.", station_name[turbocar_stop] );
      echo_to_ship( AT_CYAN, turbocar, buf );
      turbocar->hatchopen = TRUE;

      turbocar_stop++;
      if( turbocar_stop >= MAX_STATION )
         turbocar_stop = 0;
   }

}

void update_bus(  )
{
   SHIP_DATA *ship;
   SHIP_DATA *ship2;
   SHIP_DATA *target;
   int destination;
   char buf[MAX_STRING_LENGTH];

   ship = ship_from_cockpit( ROOM_SHUTTLE_BUS );
   ship2 = ship_from_cockpit( ROOM_SHUTTLE_BUS_2 );

   if( ship == NULL && ship2 == NULL )
      return;

   switch ( bus_pos )
   {

      case 0:
         target = ship_from_hanger( bus_vnum[bus_planet] );
         if( target != NULL && !target->starsystem )
         {
            sprintf( buf, "An electronic voice says, 'Cannot land at %s ... it seems to have dissapeared.'",
                     bus_stop[bus_planet] );
            echo_to_ship( AT_CYAN, ship, buf );
            bus_pos = 5;
         }

         target = ship_from_hanger( bus_vnum[bus2_planet] );
         if( target != NULL && !target->starsystem )
         {
            sprintf( buf, "An electronic voice says, 'Cannot land at %s ... it seems to have dissapeared.'",
                     bus_stop[bus_planet] );
            echo_to_ship( AT_CYAN, ship2, buf );
            bus_pos = 5;
         }

         bus_pos++;
         break;

      case 6:
         launch_bus( ship );
         launch_bus( ship2 );
         bus_pos++;
         break;

      case 7:
         echo_to_ship( AT_YELLOW, ship, "The ship lurches slightly as it makes the jump to lightspeed." );
         echo_to_ship( AT_YELLOW, ship2, "The ship lurches slightly as it makes the jump to lightspeed." );
         bus_pos++;
         break;

      case 9:

         echo_to_ship( AT_YELLOW, ship, "The ship lurches slightly as it comes out of hyperspace.." );
         echo_to_ship( AT_YELLOW, ship2, "The ship lurches slightly as it comes out of hyperspace.." );
         bus_pos++;
         break;

      case 1:
         destination = bus_vnum[bus_planet];
         if( !land_bus( ship, destination ) )
         {
            sprintf( buf, "An electronic voice says, 'Oh My, %s seems to have dissapeared.'", bus_stop[bus_planet] );
            echo_to_ship( AT_CYAN, ship, buf );
            echo_to_ship( AT_CYAN, ship, "An electronic voice says, 'I do hope it wasn't a superlaser. Landing aborted.'" );
         }
         else
         {
            sprintf( buf, "An electronic voice says, 'Welcome to %s'", bus_stop[bus_planet] );
            echo_to_ship( AT_CYAN, ship, buf );
            echo_to_ship( AT_CYAN, ship, "It continues, 'Please exit through the main ramp. Enjoy your stay.'" );
         }
         destination = bus_vnum[bus2_planet];
         if( !land_bus( ship2, destination ) )
         {
            sprintf( buf, "An electronic voice says, 'Oh My, %s seems to have dissapeared.'", bus_stop[bus_planet] );
            echo_to_ship( AT_CYAN, ship2, buf );
            echo_to_ship( AT_CYAN, ship2, "An electronic voice says, 'I do hope it wasn't a superlaser. Landing aborted.'" );
         }
         else
         {
            sprintf( buf, "An electronic voice says, 'Welcome to %s'", bus_stop[bus2_planet] );
            echo_to_ship( AT_CYAN, ship2, buf );
            echo_to_ship( AT_CYAN, ship2, "It continues, 'Please exit through the main ramp. Enjoy your stay.'" );
         }
         bus_pos++;
         break;

      case 5:
         sprintf( buf, "It continues, 'Next stop, %s'", bus_stop[bus_planet + 1] );
         echo_to_ship( AT_CYAN, ship, "An electronic voice says, 'Preparing for launch.'" );
         echo_to_ship( AT_CYAN, ship, buf );
         sprintf( buf, "It continues, 'Next stop, %s'", bus_stop[bus2_planet + 1] );
         echo_to_ship( AT_CYAN, ship2, "An electronic voice says, 'Preparing for launch.'" );
         echo_to_ship( AT_CYAN, ship2, buf );
         bus_pos++;
         break;

      default:
         bus_pos++;
         break;
   }

   if( bus_pos >= 10 )
   {
      bus_pos = 0;
      bus_planet++;
      bus2_planet++;
   }

   if( bus_planet >= MAX_BUS_STOP )
      bus_planet = 0;
   if( bus2_planet >= MAX_BUS_STOP )
      bus2_planet = 0;

}

void move_ships()
{
  SHIP_DATA *ship;
  MISSILE_DATA *missile;
  MISSILE_DATA *m_next;
  SHIP_DATA *target;
  float dx, dy, dz, change;
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *ch;
  bool ch_found = FALSE;

  for( missile = first_missile; missile; missile = m_next )
    {
      m_next = missile->next;

      ship = missile->fired_from;
      target = missile->target;

      if( target->starsystem && target->starsystem == missile->starsystem )
	{
	  if( missile->mx < target->vx )
	    missile->mx += UMIN( missile->speed / 5, ( int )( target->vx - missile->mx ) );
	  else if( missile->mx > target->vx )
            missile->mx -= UMIN( missile->speed / 5, ( int )( missile->mx - target->vx ) );
	  if( missile->my < target->vy )
            missile->my += UMIN( missile->speed / 5, ( int )( target->vy - missile->my ) );
	  else if( missile->my > target->vy )
            missile->my -= UMIN( missile->speed / 5, ( int )( missile->my - target->vy ) );
	  if( missile->mz < target->vz )
            missile->mz += UMIN( missile->speed / 5, ( int )( target->vz - missile->mz ) );
	  else if( missile->mz > target->vz )
            missile->mz -= UMIN( missile->speed / 5, ( int )( missile->mz - target->vz ) );

	  if( abs( ( int ) missile->mx ) - abs( ( int ) target->vx ) <= 20
	      && abs( ( int ) missile->mx ) - abs( ( int ) target->vx ) >= -20
	      && abs( ( int ) missile->my ) - abs( ( int ) target->vy ) <= 20
	      && abs( ( int ) missile->my ) - abs( ( int ) target->vy ) >= -20
	      && abs( ( int ) missile->mz ) - abs( ( int ) target->vz ) <= 20
	      && abs( ( int ) missile->mz ) - abs( ( int ) target->vz ) >= -20)
	    {
	      if( target->chaff_released <= 0 )
		{
		  echo_to_room( AT_YELLOW, get_room_index( ship->gunseat ), "Your missile hits its target dead on!" );
		  echo_to_cockpit( AT_BLOOD, target, "The ship is hit by a missile." );
		  echo_to_ship( AT_RED, target, "A loud explosion shakes thee ship violently!" );
		  sprintf( buf, "You see a small explosion as %s is hit by a missile", target->name );
		  echo_to_system( AT_ORANGE, target, buf, ship );
		  for( ch = first_char; ch; ch = ch->next )
		    if( !IS_NPC( ch ) && nifty_is_name( missile->fired_by, ch->name ) )
		      {
			ch_found = TRUE;
			damage_ship_ch( target, 20 + missile->missiletype * missile->missiletype * 20,
					30 + missile->missiletype * missile->missiletype * missile->missiletype * 30, ch );
		      }
		  if( !ch_found )
		    damage_ship( target, 20 + missile->missiletype * missile->missiletype * 20,
				 30 + missile->missiletype * missile->missiletype * ship->missiletype * 30 );
		  extract_missile( missile );
		}
	      else
		{
		  echo_to_room( AT_YELLOW, get_room_index( ship->gunseat ),
				"Your missile explodes harmlessly in a cloud of chaff!" );
		  echo_to_cockpit( AT_YELLOW, target, "A missile explodes in your chaff." );
		  extract_missile( missile );
		}
	      continue;
	    }
	  else
	    {
	      missile->age++;
	      if( missile->age >= 50 )
		{
		  extract_missile( missile );
		  continue;
		}
	    }
	}
      else
	{
	  extract_missile( missile );
	  continue;
	}
    }

  for( ship = first_ship; ship; ship = ship->next )
    {
      if( !ship->starsystem )
	continue;

      if( ship->currspeed > 0 )
	{
	  change = sqrt( ship->hx * ship->hx + ship->hy * ship->hy + ship->hz * ship->hz );

	  if( change > 0 )
	    {
	      dx = ship->hx / change;
	      dy = ship->hy / change;
	      dz = ship->hz / change;
	      ship->vx += ( dx * ship->currspeed / 5 );
	      ship->vy += ( dy * ship->currspeed / 5 );
	      ship->vz += ( dz * ship->currspeed / 5 );
	    }
	}

      if( autofly( ship ) )
	continue;

/*           
          if ( ship->class != SHIP_PLATFORM && !autofly(ship) )
          {
            if ( ship->starsystem->star1 && strcmp(ship->starsystem->star1,"") )
            {
              if (ship->vx >= ship->starsystem->s1x + 1 || ship->vx <= ship->starsystem->s1x - 1 )
                ship->vx -= URANGE(-3,(ship->starsystem->gravitys1)/(ship->vx - ship->starsystem->s1x)/2,3);
              if (ship->vy >= ship->starsystem->s1y + 1 || ship->vy <= ship->starsystem->s1y - 1 )
                ship->vy -= URANGE(-3,(ship->starsystem->gravitys1)/(ship->vy - ship->starsystem->s1y)/2,3);
              if (ship->vz >= ship->starsystem->s1z + 1 || ship->vz <= ship->starsystem->s1z - 1 )
                ship->vz -= URANGE(-3,(ship->starsystem->gravitys1)/(ship->vz - ship->starsystem->s1z)/2,3);
            }
          
            if ( ship->starsystem->star2 && strcmp(ship->starsystem->star2,"") )
            {
              if (ship->vx >= ship->starsystem->s2x + 1 || ship->vx <= ship->starsystem->s2x - 1 )
                ship->vx -= URANGE(-3,(ship->starsystem->gravitys2)/(ship->vx - ship->starsystem->s2x)/2,3);
              if (ship->vy >= ship->starsystem->s2y + 1 || ship->vy <= ship->starsystem->s2y - 1 )
                ship->vy -= URANGE(-3,(ship->starsystem->gravitys2)/(ship->vy - ship->starsystem->s2y)/2,3);
              if (ship->vz >= ship->starsystem->s2z + 1 || ship->vz <= ship->starsystem->s2z - 1 )
                ship->vz -= URANGE(-3,(ship->starsystem->gravitys2)/(ship->vz - ship->starsystem->s2z)/2,3);
            }
          
            if ( ship->starsystem->planet1 && strcmp(ship->starsystem->planet1,"") )
            {
              if (ship->vx >= ship->starsystem->p1x + 1 || ship->vx <= ship->starsystem->p1x - 1 )
                ship->vx -= URANGE(-3,(ship->starsystem->gravityp1)/(ship->vx - ship->starsystem->p1x)/2,3);
              if (ship->vy >= ship->starsystem->p1y + 1 || ship->vy <= ship->starsystem->p1y - 1 )
                ship->vy -= URANGE(-3,(ship->starsystem->gravityp1)/(ship->vy - ship->starsystem->p1y)/2,3);
              if (ship->vz >= ship->starsystem->p1z + 1 || ship->vz <= ship->starsystem->p1z - 1 )
                ship->vz -= URANGE(-3,(ship->starsystem->gravityp1)/(ship->vz - ship->starsystem->p1z)/2,3);
            }
          
            if ( ship->starsystem->planet2 && strcmp(ship->starsystem->planet2,"") )
            {
              if (ship->vx >= ship->starsystem->p2x + 1 || ship->vx <= ship->starsystem->p2x - 1 )
                ship->vx -= URANGE(-3,(ship->starsystem->gravityp2)/(ship->vx - ship->starsystem->p2x)/2,3);
              if (ship->vy >= ship->starsystem->p2y + 1 || ship->vy <= ship->starsystem->p2y - 1 )
                ship->vy -= URANGE(-3,(ship->starsystem->gravityp2)/(ship->vy - ship->starsystem->p2y)/2,3);
              if (ship->vz >= ship->starsystem->p2z + 1 || ship->vz <= ship->starsystem->p2z - 1 )
                ship->vz -= URANGE(-3,(ship->starsystem->gravityp2)/(ship->vz - ship->starsystem->p2z)/2,3);
            }
          
            if ( ship->starsystem->planet3 && strcmp(ship->starsystem->planet3,"") )
            {
              if (ship->vx >= ship->starsystem->p3x + 1 || ship->vx <= ship->starsystem->p3x - 1 )
                ship->vx -= URANGE(-3,(ship->starsystem->gravityp3)/(ship->vx - ship->starsystem->p3x)/2,3);
              if (ship->vy >= ship->starsystem->p3y + 1 || ship->vy <= ship->starsystem->p3y - 1 )
                ship->vy -= URANGE(-3,(ship->starsystem->gravityp3)/(ship->vy - ship->starsystem->p3y)/2,3);
              if (ship->vz >= ship->starsystem->p3z + 1 || ship->vz <= ship->starsystem->p3z - 1 )
                ship->vz -= URANGE(-3,(ship->starsystem->gravityp3)/(ship->vz - ship->starsystem->p3z)/2,3);
            }
          }

*/
/*
          for ( target = ship->starsystem->first_ship; target; target = target->next_in_starsystem)
          { 
                if ( target != ship &&
                    abs(ship->vx - target->vx) < 1 &&
                    abs(ship->vy - target->vy) < 1 &&
                    abs(ship->vz - target->vz) < 1 )
                {
                    ship->collision = target->maxhull;
                    target->collision = ship->maxhull;
                }                   
          }
*/
      if( ship->starsystem->star1 && strcmp( ship->starsystem->star1, "" )
	  && abs( ( int )( ship->vx - ship->starsystem->s1x ) ) < 10
	  && abs( ( int )( ship->vy - ship->starsystem->s1y ) ) < 10
	  && abs( ( int )( ship->vz - ship->starsystem->s1z ) ) < 10 )
	{
	  echo_to_cockpit( AT_BLOOD + AT_BLINK, ship, "You fly directly into the sun." );
	  sprintf( buf, "%s flys directly into %s!", ship->name, ship->starsystem->star1 );
	  echo_to_system( AT_ORANGE, ship, buf, NULL );
	  destroy_ship( ship, NULL );
	  continue;
	}
      if( ship->starsystem->star2 && strcmp( ship->starsystem->star2, "" )
	  && abs( ( int )( ship->vx - ship->starsystem->s2x ) ) < 10
	  && abs( ( int )( ship->vy - ship->starsystem->s2y ) ) < 10
	  && abs( ( int )( ship->vz - ship->starsystem->s2z ) ) < 10 )
	{
	  echo_to_cockpit( AT_BLOOD + AT_BLINK, ship, "You fly directly into the sun." );
	  sprintf( buf, "%s flys directly into %s!", ship->name, ship->starsystem->star2 );
	  echo_to_system( AT_ORANGE, ship, buf, NULL );
	  destroy_ship( ship, NULL );
	  continue;
	}

      if( ship->currspeed > 0 )
	{
	  if( ship->starsystem->planet1 && strcmp( ship->starsystem->planet1, "" )
	      && abs( ( int )( ship->vx - ship->starsystem->p1x ) ) < 10
	      && abs( ( int )( ship->vy - ship->starsystem->p1y ) ) < 10
	      && abs( ( int )( ship->vz - ship->starsystem->p1z ) ) < 10 )
	    {
	      sprintf( buf, "You begin orbitting %s.", ship->starsystem->planet1 );
	      echo_to_cockpit( AT_YELLOW, ship, buf );
	      sprintf( buf, "%s begins orbiting %s.", ship->name, ship->starsystem->planet1 );
	      echo_to_system( AT_ORANGE, ship, buf, NULL );
	      ship->currspeed = 0;
	      continue;
	    }
	  if( ship->starsystem->planet2 && strcmp( ship->starsystem->planet2, "" )
	      && abs( ( int )( ship->vx - ship->starsystem->p2x ) ) < 10
	      && abs( ( int )( ship->vy - ship->starsystem->p2y ) ) < 10
	      && abs( ( int )( ship->vz - ship->starsystem->p2z ) ) < 10 )
	    {
	      sprintf( buf, "You begin orbitting %s.", ship->starsystem->planet2 );
	      echo_to_cockpit( AT_YELLOW, ship, buf );
	      sprintf( buf, "%s begins orbiting %s.", ship->name, ship->starsystem->planet2 );
	      echo_to_system( AT_ORANGE, ship, buf, NULL );
	      ship->currspeed = 0;
	      continue;
	    }
	  if( ship->starsystem->planet3 && strcmp( ship->starsystem->planet3, "" )
	      && abs( ( int )( ship->vx - ship->starsystem->p3x ) ) < 10
	      && abs( ( int )( ship->vy - ship->starsystem->p3y ) ) < 10
	      && abs( ( int )( ship->vz - ship->starsystem->p3z ) ) < 10 )
	    {
	      sprintf( buf, "You begin orbitting %s.", ship->starsystem->planet2 );
	      echo_to_cockpit( AT_YELLOW, ship, buf );
	      sprintf( buf, "%s begins orbiting %s.", ship->name, ship->starsystem->planet2 );
	      echo_to_system( AT_ORANGE, ship, buf, NULL );
	      ship->currspeed = 0;
	      continue;
	    }
	}
    }

  for( ship = first_ship; ship; ship = ship->next )
    if( ship->collision )
      {
	echo_to_cockpit( AT_WHITE + AT_BLINK, ship, "You have collided with another ship!" );
	echo_to_ship( AT_RED, ship, "A loud explosion shakes the ship violently!" );
	damage_ship( ship, ship->collision, ship->collision );
	ship->collision = 0;
      }
}

void recharge_ships(  )
{
   SHIP_DATA *ship;
   char buf[MAX_STRING_LENGTH];

   for( ship = first_ship; ship; ship = ship->next )
   {

      if( ship->statet0 > 0 )
      {
         ship->energy -= ship->statet0;
         ship->statet0 = 0;
      }
      if( ship->statet1 > 0 )
      {
         ship->energy -= ship->statet1;
         ship->statet1 = 0;
      }
      if( ship->statet2 > 0 )
      {
         ship->energy -= ship->statet2;
         ship->statet2 = 0;
      }

      if( ship->missilestate == MISSILE_RELOAD_2 )
      {
         ship->missilestate = MISSILE_READY;
         if( ship->missiles > 0 )
            echo_to_room( AT_YELLOW, get_room_index( ship->gunseat ), "Missile launcher reloaded." );
      }

      if( ship->missilestate == MISSILE_RELOAD )
      {
         ship->missilestate = MISSILE_RELOAD_2;
      }

      if( ship->missilestate == MISSILE_FIRED )
         ship->missilestate = MISSILE_RELOAD;

      if( autofly( ship ) )
      {
         if( ship->starsystem )
         {
            if( ship->target0 && ship->statet0 != LASER_DAMAGED )
            {
               int schance = 50;
               SHIP_DATA *target = ship->target0;
               int shots;

               for( shots = 0; shots <= ship->lasers; shots++ )
               {
                  if( ship->shipstate != SHIP_HYPERSPACE && ship->energy > 25
                      && ship->target0->starsystem == ship->starsystem
                      && abs( ( int )( target->vx - ship->vx ) ) <= 1000
                      && abs( ( int )( target->vy - ship->vy ) ) <= 1000
                      && abs( ( int )( target->vz - ship->vz ) ) <= 1000
		      && ship->statet0 < ship->lasers )
                  {
                     if( ship->ship_class > 1 || is_facing( ship, target ) )
                     {
                        schance += target->ship_class * 25;
                        schance -= target->manuever / 10;
                        schance -= target->currspeed / 20;
                        schance -= ( abs( ( int )( target->vx - ship->vx ) ) / 70 );
                        schance -= ( abs( ( int )( target->vy - ship->vy ) ) / 70 );
                        schance -= ( abs( ( int )( target->vz - ship->vz ) ) / 70 );
                        schance = URANGE( 10, schance, 90 );
                        if( number_percent(  ) > schance )
                        {
                           sprintf( buf, "%s fires at you but misses.", ship->name );
                           echo_to_cockpit( AT_ORANGE, target, buf );
                           sprintf( buf, "Laserfire from %s barely misses %s.", ship->name, target->name );
                           echo_to_system( AT_ORANGE, target, buf, NULL );
                        }
                        else
                        {
                           sprintf( buf, "Laserfire from %s hits %s.", ship->name, target->name );
                           echo_to_system( AT_ORANGE, target, buf, NULL );
                           sprintf( buf, "You are hit by lasers from %s!", ship->name );
                           echo_to_cockpit( AT_BLOOD, target, buf );
                           echo_to_ship( AT_RED, target, "A small explosion vibrates through the ship." );
                           damage_ship( target, 5, 10 );
                        }
                        ship->statet0++;
                     }
                  }
               }
            }
         }
      }

   }
}

void update_space(  )
{
   SHIP_DATA *ship;
   SHIP_DATA *target;
   char buf[MAX_STRING_LENGTH];
   int too_close, target_too_close;
   int recharge;

   for( ship = first_ship; ship; ship = ship->next )
   {
      if( ship->starsystem )
      {
         if( ship->energy > 0 && ship->shipstate == SHIP_DISABLED
	     && ship->ship_class != SHIP_PLATFORM )
            ship->energy -= 100;
         else if( ship->energy > 0 )
            ship->energy += ( 5 + ship->ship_class * 5 );
         else
            destroy_ship( ship, NULL );
      }

      if( ship->chaff_released > 0 )
         ship->chaff_released--;

      if( ship->shipstate == SHIP_HYPERSPACE )
      {
         ship->hyperdistance -= ship->hyperspeed * 2;
         if( ship->hyperdistance <= 0 )
         {
            ship_to_starsystem( ship, ship->currjump );

            if( ship->starsystem == NULL )
            {
               echo_to_cockpit( AT_RED, ship, "Ship lost in Hyperspace. Make new calculations." );
            }
            else
            {
               echo_to_room( AT_YELLOW, get_room_index( ship->pilotseat ), "Hyperjump complete." );
               echo_to_ship( AT_YELLOW, ship, "The ship lurches slightly as it comes out of hyperspace." );
               sprintf( buf, "%s enters the starsystem at %.0f %.0f %.0f", ship->name, ship->vx, ship->vy, ship->vz );
               echo_to_system( AT_YELLOW, ship, buf, NULL );
               ship->shipstate = SHIP_READY;
               STRFREE( ship->home );
               ship->home = STRALLOC( ship->starsystem->name );
               if( str_cmp( "Public", ship->owner ) )
                  save_ship( ship );

            }
         }
         else
         {
            sprintf( buf, "%d", ship->hyperdistance );
            echo_to_room_dnr( AT_YELLOW, get_room_index( ship->pilotseat ), "Remaining jump distance: " );
            echo_to_room( AT_WHITE, get_room_index( ship->pilotseat ), buf );

         }
      }

      /*
       * following was originaly to fix ships that lost their pilot 
       * in the middle of a manuever and are stuck in a busy state 
       * but now used for timed manouevers such as turning 
       */

      if( ship->shipstate == SHIP_BUSY_3 )
      {
         echo_to_room( AT_YELLOW, get_room_index( ship->pilotseat ), "Manuever complete." );
         ship->shipstate = SHIP_READY;
      }
      if( ship->shipstate == SHIP_BUSY_2 )
         ship->shipstate = SHIP_BUSY_3;
      if( ship->shipstate == SHIP_BUSY )
         ship->shipstate = SHIP_BUSY_2;

      if( ship->shipstate == SHIP_LAND_2 )
         landship( ship, ship->dest );
      if( ship->shipstate == SHIP_LAND )
         ship->shipstate = SHIP_LAND_2;

      if( ship->shipstate == SHIP_LAUNCH_2 )
         launchship( ship );
      if( ship->shipstate == SHIP_LAUNCH )
         ship->shipstate = SHIP_LAUNCH_2;


      ship->shield = UMAX( 0, ship->shield - 1 - ship->ship_class );

      if( ship->autorecharge && ship->maxshield > ship->shield && ship->energy > 100 )
      {
         recharge = UMIN( ship->maxshield - ship->shield, 10 + ship->ship_class * 10 );
         recharge = UMIN( recharge, ship->energy / 2 - 100 );
         recharge = UMAX( 1, recharge );
         ship->shield += recharge;
         ship->energy -= recharge;
      }

      if( ship->shield > 0 )
      {
         if( ship->energy < 200 )
         {
            ship->shield = 0;
            echo_to_cockpit( AT_RED, ship, "The ships shields fizzle and die." );
            ship->autorecharge = FALSE;
         }
      }

      if( ship->starsystem && ship->currspeed > 0 )
      {
         sprintf( buf, "%d", ship->currspeed );
         echo_to_room_dnr( AT_BLUE, get_room_index( ship->pilotseat ), "Speed: " );
         echo_to_room_dnr( AT_LBLUE, get_room_index( ship->pilotseat ), buf );
         sprintf( buf, "%.0f %.0f %.0f", ship->vx, ship->vy, ship->vz );
         echo_to_room_dnr( AT_BLUE, get_room_index( ship->pilotseat ), "  Coords: " );
         echo_to_room( AT_LBLUE, get_room_index( ship->pilotseat ), buf );
         if( ship->pilotseat != ship->coseat )
         {
            sprintf( buf, "%d", ship->currspeed );
            echo_to_room_dnr( AT_BLUE, get_room_index( ship->coseat ), "Speed: " );
            echo_to_room_dnr( AT_LBLUE, get_room_index( ship->coseat ), buf );
            sprintf( buf, "%.0f %.0f %.0f", ship->vx, ship->vy, ship->vz );
            echo_to_room_dnr( AT_BLUE, get_room_index( ship->coseat ), "  Coords: " );
            echo_to_room( AT_LBLUE, get_room_index( ship->coseat ), buf );
         }
      }

      if( ship->starsystem )
      {
         too_close = ship->currspeed + 50;
         for( target = ship->starsystem->first_ship; target; target = target->next_in_starsystem )
         {
            target_too_close = too_close + target->currspeed;
            if( target != ship
		&& abs( ( int )( ship->vx - target->vx ) ) < target_too_close
		&& abs( ( int )( ship->vy - target->vy ) ) < target_too_close
		&& abs( ( int )( ship->vz - target->vz ) ) < target_too_close )
            {
               sprintf( buf, "Proximity alert: %s  %.0f %.0f %.0f", target->name, target->vx, target->vy, target->vz );
               echo_to_room( AT_RED, get_room_index( ship->pilotseat ), buf );
            }
         }
         too_close = ship->currspeed + 100;
         if( ship->starsystem->star1 && strcmp( ship->starsystem->star1, "" )
	     && abs( ( int )( ship->vx - ship->starsystem->s1x ) ) < too_close
	     && abs( ( int )( ship->vy - ship->starsystem->s1y ) ) < too_close
	     && abs( ( int )( ship->vz - ship->starsystem->s1z ) ) < too_close)
         {
            sprintf( buf, "Proximity alert: %s  %d %d %d", ship->starsystem->star1,
                     ship->starsystem->s1x, ship->starsystem->s1y, ship->starsystem->s1z );
            echo_to_room( AT_RED, get_room_index( ship->pilotseat ), buf );
         }
         if( ship->starsystem->star2 && strcmp( ship->starsystem->star2, "" )
	     && abs( ( int )( ship->vx - ship->starsystem->s2x ) ) < too_close
	     && abs( ( int )( ship->vy - ship->starsystem->s2y ) ) < too_close
	     && abs( ( int )( ship->vz - ship->starsystem->s2z ) ) < too_close)
         {
            sprintf( buf, "Proximity alert: %s  %d %d %d", ship->starsystem->star2,
                     ship->starsystem->s2x, ship->starsystem->s2y, ship->starsystem->s2z );
            echo_to_room( AT_RED, get_room_index( ship->pilotseat ), buf );
         }
         if( ship->starsystem->planet1
	     && strcmp( ship->starsystem->planet1, "" )
	     && abs( ( int )( ship->vx - ship->starsystem->p1x ) ) < too_close
	     && abs( ( int )( ship->vy - ship->starsystem->p1y ) ) < too_close
	     && abs( ( int )( ship->vz - ship->starsystem->p1z ) ) < too_close)
         {
            sprintf( buf, "Proximity alert: %s  %d %d %d", ship->starsystem->planet1,
                     ship->starsystem->p1x, ship->starsystem->p1y, ship->starsystem->p1z );
            echo_to_room( AT_RED, get_room_index( ship->pilotseat ), buf );
         }
         if( ship->starsystem->planet2
	     && strcmp( ship->starsystem->planet2, "" )
	     && abs( ( int )( ship->vx - ship->starsystem->p2x ) ) < too_close
	     && abs( ( int )( ship->vy - ship->starsystem->p2y ) ) < too_close
	     && abs( ( int )( ship->vz - ship->starsystem->p2z ) ) < too_close)
         {
            sprintf( buf, "Proximity alert: %s  %d %d %d", ship->starsystem->planet2,
                     ship->starsystem->p2x, ship->starsystem->p2y, ship->starsystem->p2z );
            echo_to_room( AT_RED, get_room_index( ship->pilotseat ), buf );
         }
         if( ship->starsystem->planet3
	     && strcmp( ship->starsystem->planet3, "" )
	     && abs( ( int )( ship->vx - ship->starsystem->p3x ) ) < too_close
	     && abs( ( int )( ship->vy - ship->starsystem->p3y ) ) < too_close
	     && abs( ( int )( ship->vz - ship->starsystem->p3z ) ) < too_close)
         {
            sprintf( buf, "Proximity alert: %s  %d %d %d", ship->starsystem->planet3,
                     ship->starsystem->p3x, ship->starsystem->p3y, ship->starsystem->p3z );
            echo_to_room( AT_RED, get_room_index( ship->pilotseat ), buf );
         }
      }


      if( ship->target0 )
      {
         sprintf( buf, "%s   %.0f %.0f %.0f", ship->target0->name, ship->target0->vx, ship->target0->vy, ship->target0->vz );
         echo_to_room_dnr( AT_BLUE, get_room_index( ship->gunseat ), "Target: " );
         echo_to_room( AT_LBLUE, get_room_index( ship->gunseat ), buf );
         if( ship->starsystem != ship->target0->starsystem )
            ship->target0 = NULL;
      }

      if( ship->target1 )
      {
         sprintf( buf, "%s   %.0f %.0f %.0f", ship->target1->name, ship->target1->vx, ship->target1->vy, ship->target1->vz );
         echo_to_room_dnr( AT_BLUE, get_room_index( ship->turret1 ), "Target: " );
         echo_to_room( AT_LBLUE, get_room_index( ship->turret1 ), buf );
         if( ship->starsystem != ship->target1->starsystem )
            ship->target1 = NULL;
      }

      if( ship->target2 )
      {
         sprintf( buf, "%s   %.0f %.0f %.0f", ship->target2->name, ship->target2->vx, ship->target2->vy, ship->target2->vz );
         echo_to_room_dnr( AT_BLUE, get_room_index( ship->turret2 ), "Target: " );
         echo_to_room( AT_LBLUE, get_room_index( ship->turret2 ), buf );
         if( ship->starsystem != ship->target2->starsystem )
            ship->target2 = NULL;
      }

      if( ship->energy < 100 && ship->starsystem )
      {
         echo_to_cockpit( AT_RED, ship, "Warning: Ship fuel low." );
      }

      ship->energy = URANGE( 0, ship->energy, ship->maxenergy );
   }

   for( ship = first_ship; ship; ship = ship->next )
   {

      if( ship->autotrack && ship->target0 && ship->ship_class < 3 )
      {
         target = ship->target0;
         too_close = ship->currspeed + 10;
         target_too_close = too_close + target->currspeed;
         if( target != ship && ship->shipstate == SHIP_READY
	     && abs( ( int )( ship->vx - target->vx ) ) < target_too_close
	     && abs( ( int )( ship->vy - target->vy ) ) < target_too_close
	     && abs( ( int )( ship->vz - target->vz ) ) < target_too_close )
         {
            ship->hx = 0 - ( ship->target0->vx - ship->vx );
            ship->hy = 0 - ( ship->target0->vy - ship->vy );
            ship->hz = 0 - ( ship->target0->vz - ship->vz );
            ship->energy -= ship->currspeed / 10;
            echo_to_room( AT_RED, get_room_index( ship->pilotseat ), "Autotrack: Evading to avoid collision!\r\n" );
            if( ship->ship_class == FIGHTER_SHIP
		|| ( ship->ship_class == MIDSIZE_SHIP
		     && ship->manuever > 50 ) )
               ship->shipstate = SHIP_BUSY_3;
            else if( ship->ship_class == MIDSIZE_SHIP
		     || ( ship->ship_class == CAPITAL_SHIP
			  && ship->manuever > 50 ) )
               ship->shipstate = SHIP_BUSY_2;
            else
               ship->shipstate = SHIP_BUSY;
         }
         else if( !is_facing( ship, ship->target0 ) )
         {
            ship->hx = ship->target0->vx - ship->vx;
            ship->hy = ship->target0->vy - ship->vy;
            ship->hz = ship->target0->vz - ship->vz;
            ship->energy -= ship->currspeed / 10;
            echo_to_room( AT_BLUE, get_room_index( ship->pilotseat ), "Autotracking target ... setting new course.\r\n" );
            if( ship->ship_class == FIGHTER_SHIP || ( ship->ship_class == MIDSIZE_SHIP && ship->manuever > 50 ) )
               ship->shipstate = SHIP_BUSY_3;
            else if( ship->ship_class == MIDSIZE_SHIP || ( ship->ship_class == CAPITAL_SHIP && ship->manuever > 50 ) )
               ship->shipstate = SHIP_BUSY_2;
            else
               ship->shipstate = SHIP_BUSY;
         }
      }

      if( autofly( ship ) )
      {
         if( ship->starsystem )
         {
            if( ship->target0 )
            {
               int schance = 50;

               /*
                * auto assist ships 
                */

               for( target = ship->starsystem->first_ship; target; target = target->next_in_starsystem )
               {
                  if( autofly( target ) )
                     if( !str_cmp( target->owner, ship->owner ) && target != ship )
                        if( target->target0 == NULL && ship->target0 != target )
                        {
                           target->target0 = ship->target0;
                           sprintf( buf, "You are being targetted by %s.", target->name );
                           echo_to_cockpit( AT_BLOOD, target->target0, buf );
                           break;
                        }
               }

               target = ship->target0;
               ship->autotrack = TRUE;
               if( ship->ship_class != SHIP_PLATFORM )
                  ship->currspeed = ship->realspeed;
               if( ship->energy > 200 )
                  ship->autorecharge = TRUE;


               if( ship->shipstate != SHIP_HYPERSPACE && ship->energy > 25
                   && ship->missilestate == MISSILE_READY && ship->target0->starsystem == ship->starsystem
                   && abs( ( int )( target->vx - ship->vx ) ) <= 1200
                   && abs( ( int )( target->vy - ship->vy ) ) <= 1200
		   && abs( ( int )( target->vz - ship->vz ) ) <= 1200
		   && ship->missiles > 0 )
               {
                  if( ship->ship_class > 1 || is_facing( ship, target ) )
                  {
                     schance -= target->manuever / 5;
                     schance -= target->currspeed / 20;
                     schance += target->ship_class * target->ship_class * 25;
                     schance -= ( abs( ( int )( target->vx - ship->vx ) ) / 100 );
                     schance -= ( abs( ( int )( target->vy - ship->vy ) ) / 100 );
                     schance -= ( abs( ( int )( target->vz - ship->vz ) ) / 100 );
                     schance += ( 30 );
                     schance = URANGE( 10, schance, 90 );

                     if( number_percent(  ) > schance )
                     {
                     }
                     else
                     {
                        new_missile( ship, target, NULL, CONCUSSION_MISSILE );
                        ship->missiles--;
                        sprintf( buf, "Incoming missile from %s.", ship->name );
                        echo_to_cockpit( AT_BLOOD, target, buf );
                        sprintf( buf, "%s fires a missile towards %s.", ship->name, target->name );
                        echo_to_system( AT_ORANGE, target, buf, NULL );

                        if( ship->ship_class == CAPITAL_SHIP
			    || ship->ship_class == SHIP_PLATFORM )
                           ship->missilestate = MISSILE_RELOAD_2;
                        else
                           ship->missilestate = MISSILE_FIRED;
                     }
                  }
               }

               if( ship->missilestate == MISSILE_DAMAGED )
                  ship->missilestate = MISSILE_READY;
               if( ship->statet0 == LASER_DAMAGED )
                  ship->statet0 = LASER_READY;
               if( ship->shipstate == SHIP_DISABLED )
                  ship->shipstate = SHIP_READY;

            }
            else
            {
               ship->currspeed = 0;

               if( !str_cmp( ship->owner, "The Empire" ) )
                  for( target = first_ship; target; target = target->next )
                     if( ship->starsystem == target->starsystem )
                        if( !str_cmp( target->owner, "The New Republic" ) )
                        {
                           ship->target0 = target;
                           sprintf( buf, "You are being targetted by %s.", ship->name );
                           echo_to_cockpit( AT_BLOOD, target, buf );
                           break;
                        }
               if( !str_cmp( ship->owner, "The New Republic" ) )
                  for( target = first_ship; target; target = target->next )
                     if( ship->starsystem == target->starsystem )
                        if( !str_cmp( target->owner, "The Empire" ) )
                        {
                           sprintf( buf, "You are being targetted by %s.", ship->name );
                           echo_to_cockpit( AT_BLOOD, target, buf );
                           ship->target0 = target;
                           break;
                        }

               if( !str_cmp( ship->owner, "Pirates" ) )
                  for( target = first_ship; target; target = target->next )
                     if( ship->starsystem == target->starsystem )
                     {
                        sprintf( buf, "You are being targetted by %s.", ship->name );
                        echo_to_cockpit( AT_BLOOD, target, buf );
                        ship->target0 = target;
                        break;
                     }

            }
         }
         else
         {
            if( number_range( 1, 25 ) == 25 )
            {
               ship_to_starsystem( ship, starsystem_from_name( ship->home ) );
               ship->vx = number_range( -5000, 5000 );
               ship->vy = number_range( -5000, 5000 );
               ship->vz = number_range( -5000, 5000 );
               ship->hx = 1;
               ship->hy = 1;
               ship->hz = 1;
            }
         }
      }

      if( ( ship->ship_class == CAPITAL_SHIP
	    || ship->ship_class == SHIP_PLATFORM ) && ship->target0 == NULL )
      {
         if( ship->missiles < ship->maxmissiles )
            ship->missiles++;
         if( ship->torpedos < ship->maxtorpedos )
            ship->torpedos++;
         if( ship->rockets < ship->maxrockets )
            ship->rockets++;
      }
   }

}



void write_starsystem_list(  )
{
   SPACE_DATA *tstarsystem;
   FILE *fpout;
   char filename[256];

   sprintf( filename, "%s%s", SPACE_DIR, SPACE_LIST );
   fpout = fopen( filename, "w" );
   if( !fpout )
   {
      bug( "FATAL: cannot open starsystem.lst for writing!\r\n", 0 );
      return;
   }
   for( tstarsystem = first_starsystem; tstarsystem; tstarsystem = tstarsystem->next )
      fprintf( fpout, "%s\n", tstarsystem->filename );
   fprintf( fpout, "$\n" );
   fclose( fpout );
}


/*
 * Get pointer to space structure from starsystem name.
 */
SPACE_DATA *starsystem_from_name( const char *name )
{
   SPACE_DATA *starsystem;

   for( starsystem = first_starsystem; starsystem; starsystem = starsystem->next )
      if( !str_cmp( name, starsystem->name ) )
         return starsystem;

   for( starsystem = first_starsystem; starsystem; starsystem = starsystem->next )
      if( !str_prefix( name, starsystem->name ) )
         return starsystem;

   return NULL;
}

/*
 * Get pointer to space structure from the dock vnun.
 */
SPACE_DATA *starsystem_from_vnum( int vnum )
{
   SPACE_DATA *starsystem;
   SHIP_DATA *ship;

   for( starsystem = first_starsystem; starsystem; starsystem = starsystem->next )
      if( vnum == starsystem->doc1a || vnum == starsystem->doc2a || vnum == starsystem->doc3a ||
          vnum == starsystem->doc1b || vnum == starsystem->doc2b || vnum == starsystem->doc3b ||
          vnum == starsystem->doc1c || vnum == starsystem->doc2c || vnum == starsystem->doc3c )
         return starsystem;

   for( ship = first_ship; ship; ship = ship->next )
      if( vnum == ship->hanger )
         return ship->starsystem;

   return NULL;
}


/*
 * Save a starsystem's data to its data file
 */
void save_starsystem( SPACE_DATA * starsystem )
{
   FILE *fp;
   char filename[256];
   char buf[MAX_STRING_LENGTH];

   if( !starsystem )
   {
      bug( "save_starsystem: null starsystem pointer!", 0 );
      return;
   }

   if( !starsystem->filename || starsystem->filename[0] == '\0' )
   {
      sprintf( buf, "save_starsystem: %s has no filename", starsystem->name );
      bug( buf, 0 );
      return;
   }

   sprintf( filename, "%s%s", SPACE_DIR, starsystem->filename );

   if( ( fp = fopen( filename, "w" ) ) == NULL )
   {
      bug( "save_starsystem: fopen", 0 );
      perror( filename );
   }
   else
   {
      fprintf( fp, "#SPACE\n" );
      fprintf( fp, "Name         %s~\n", starsystem->name );
      fprintf( fp, "Filename     %s~\n", starsystem->filename );
      fprintf( fp, "Planet1      %s~\n", starsystem->planet1 );
      fprintf( fp, "Planet2      %s~\n", starsystem->planet2 );
      fprintf( fp, "Planet3      %s~\n", starsystem->planet3 );
      fprintf( fp, "Star1        %s~\n", starsystem->star1 );
      fprintf( fp, "Star2        %s~\n", starsystem->star2 );
      fprintf( fp, "Location1a      %s~\n", starsystem->location1a );
      fprintf( fp, "Location1b      %s~\n", starsystem->location1b );
      fprintf( fp, "Location1c      %s~\n", starsystem->location1c );
      fprintf( fp, "Location2a       %s~\n", starsystem->location2a );
      fprintf( fp, "Location2b      %s~\n", starsystem->location2b );
      fprintf( fp, "Location2c      %s~\n", starsystem->location2c );
      fprintf( fp, "Location3a      %s~\n", starsystem->location3a );
      fprintf( fp, "Location3b      %s~\n", starsystem->location3b );
      fprintf( fp, "Location3c      %s~\n", starsystem->location3c );
      fprintf( fp, "Doc1a          %d\n", starsystem->doc1a );
      fprintf( fp, "Doc2a          %d\n", starsystem->doc2a );
      fprintf( fp, "Doc3a          %d\n", starsystem->doc3a );
      fprintf( fp, "Doc1b          %d\n", starsystem->doc1b );
      fprintf( fp, "Doc2b          %d\n", starsystem->doc2b );
      fprintf( fp, "Doc3b          %d\n", starsystem->doc3b );
      fprintf( fp, "Doc1c          %d\n", starsystem->doc1c );
      fprintf( fp, "Doc2c          %d\n", starsystem->doc2c );
      fprintf( fp, "Doc3c          %d\n", starsystem->doc3c );
      fprintf( fp, "P1x          %d\n", starsystem->p1x );
      fprintf( fp, "P1y          %d\n", starsystem->p1y );
      fprintf( fp, "P1z          %d\n", starsystem->p1z );
      fprintf( fp, "P2x          %d\n", starsystem->p2x );
      fprintf( fp, "P2y          %d\n", starsystem->p2y );
      fprintf( fp, "P2z          %d\n", starsystem->p2z );
      fprintf( fp, "P3x          %d\n", starsystem->p3x );
      fprintf( fp, "P3y          %d\n", starsystem->p3y );
      fprintf( fp, "P3z          %d\n", starsystem->p3z );
      fprintf( fp, "S1x          %d\n", starsystem->s1x );
      fprintf( fp, "S1y          %d\n", starsystem->s1y );
      fprintf( fp, "S1z          %d\n", starsystem->s1z );
      fprintf( fp, "S2x          %d\n", starsystem->s2x );
      fprintf( fp, "S2y          %d\n", starsystem->s2y );
      fprintf( fp, "S2z          %d\n", starsystem->s2z );
      fprintf( fp, "Gravitys1     %d\n", starsystem->gravitys1 );
      fprintf( fp, "Gravitys2     %d\n", starsystem->gravitys2 );
      fprintf( fp, "Gravityp1     %d\n", starsystem->gravityp1 );
      fprintf( fp, "Gravityp2     %d\n", starsystem->gravityp2 );
      fprintf( fp, "Gravityp3     %d\n", starsystem->gravityp3 );
      fprintf( fp, "Xpos          %d\n", starsystem->xpos );
      fprintf( fp, "Ypos          %d\n", starsystem->ypos );
      fprintf( fp, "End\n\n" );
      fprintf( fp, "#END\n" );
      fclose( fp );
      fp = NULL;
   }
   return;
}

/*
 * Read in actual starsystem data.
 */
void fread_starsystem( SPACE_DATA * starsystem, FILE * fp )
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

         case 'D':
            KEY( "Doc1a", starsystem->doc1a, fread_number( fp ) );
            KEY( "Doc2a", starsystem->doc2a, fread_number( fp ) );
            KEY( "Doc3a", starsystem->doc3a, fread_number( fp ) );
            KEY( "Doc1b", starsystem->doc1b, fread_number( fp ) );
            KEY( "Doc2b", starsystem->doc2b, fread_number( fp ) );
            KEY( "Doc3b", starsystem->doc3b, fread_number( fp ) );
            KEY( "Doc1c", starsystem->doc1c, fread_number( fp ) );
            KEY( "Doc2c", starsystem->doc2c, fread_number( fp ) );
            KEY( "Doc3c", starsystem->doc3c, fread_number( fp ) );
            break;


         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( !starsystem->name )
                  starsystem->name = STRALLOC( "" );
               if( !starsystem->location1a )
                  starsystem->location1a = STRALLOC( "" );
               if( !starsystem->location2a )
                  starsystem->location2a = STRALLOC( "" );
               if( !starsystem->location3a )
                  starsystem->location3a = STRALLOC( "" );
               if( !starsystem->location1b )
                  starsystem->location1b = STRALLOC( "" );
               if( !starsystem->location2b )
                  starsystem->location2b = STRALLOC( "" );
               if( !starsystem->location3b )
                  starsystem->location3b = STRALLOC( "" );
               if( !starsystem->location1c )
                  starsystem->location1c = STRALLOC( "" );
               if( !starsystem->location2c )
                  starsystem->location2c = STRALLOC( "" );
               if( !starsystem->location3c )
                  starsystem->location3c = STRALLOC( "" );
               if( !starsystem->planet1 )
                  starsystem->planet1 = STRALLOC( "" );
               if( !starsystem->planet2 )
                  starsystem->planet2 = STRALLOC( "" );
               if( !starsystem->planet3 )
                  starsystem->planet3 = STRALLOC( "" );
               if( !starsystem->star1 )
                  starsystem->star1 = STRALLOC( "" );
               if( !starsystem->star2 )
                  starsystem->star2 = STRALLOC( "" );
               return;
            }
            break;

         case 'F':
            KEY( "Filename", starsystem->filename, fread_string_nohash( fp ) );
            break;

         case 'G':
            KEY( "Gravitys1", starsystem->gravitys1, fread_number( fp ) );
            KEY( "Gravitys2", starsystem->gravitys2, fread_number( fp ) );
            KEY( "Gravityp1", starsystem->gravityp1, fread_number( fp ) );
            KEY( "Gravityp2", starsystem->gravityp2, fread_number( fp ) );
            KEY( "Gravityp3", starsystem->gravityp3, fread_number( fp ) );
            break;

         case 'L':
            KEY( "Location1a", starsystem->location1a, fread_string( fp ) );
            KEY( "Location2a", starsystem->location2a, fread_string( fp ) );
            KEY( "Location3a", starsystem->location3a, fread_string( fp ) );
            KEY( "Location1b", starsystem->location1b, fread_string( fp ) );
            KEY( "Location2b", starsystem->location2b, fread_string( fp ) );
            KEY( "Location3b", starsystem->location3b, fread_string( fp ) );
            KEY( "Location1c", starsystem->location1c, fread_string( fp ) );
            KEY( "Location2c", starsystem->location2c, fread_string( fp ) );
            KEY( "Location3c", starsystem->location3c, fread_string( fp ) );
            break;

         case 'N':
            KEY( "Name", starsystem->name, fread_string( fp ) );
            break;

         case 'P':
            KEY( "Planet1", starsystem->planet1, fread_string( fp ) );
            KEY( "Planet2", starsystem->planet2, fread_string( fp ) );
            KEY( "Planet3", starsystem->planet3, fread_string( fp ) );
            KEY( "P1x", starsystem->p1x, fread_number( fp ) );
            KEY( "P1y", starsystem->p1y, fread_number( fp ) );
            KEY( "P1z", starsystem->p1z, fread_number( fp ) );
            KEY( "P2x", starsystem->p2x, fread_number( fp ) );
            KEY( "P2y", starsystem->p2y, fread_number( fp ) );
            KEY( "P2z", starsystem->p2z, fread_number( fp ) );
            KEY( "P3x", starsystem->p3x, fread_number( fp ) );
            KEY( "P3y", starsystem->p3y, fread_number( fp ) );
            KEY( "P3z", starsystem->p3z, fread_number( fp ) );
            break;

         case 'S':
            KEY( "Star1", starsystem->star1, fread_string( fp ) );
            KEY( "Star2", starsystem->star2, fread_string( fp ) );
            KEY( "S1x", starsystem->s1x, fread_number( fp ) );
            KEY( "S1y", starsystem->s1y, fread_number( fp ) );
            KEY( "S1z", starsystem->s1z, fread_number( fp ) );
            KEY( "S2x", starsystem->s2x, fread_number( fp ) );
            KEY( "S2y", starsystem->s2y, fread_number( fp ) );
            KEY( "S2z", starsystem->s2z, fread_number( fp ) );

         case 'X':
            KEY( "Xpos", starsystem->xpos, fread_number( fp ) );

         case 'Y':
            KEY( "Ypos", starsystem->ypos, fread_number( fp ) );

      }

      if( !fMatch )
      {
         sprintf( buf, "Fread_starsystem: no match: %s", word );
         bug( buf, 0 );
      }
   }
}

/*
 * Load a starsystem file
 */

bool load_starsystem( const char *starsystemfile )
{
   char filename[256];
   SPACE_DATA *starsystem;
   FILE *fp;
   bool found;

   CREATE( starsystem, SPACE_DATA, 1 );

   found = FALSE;
   sprintf( filename, "%s%s", SPACE_DIR, starsystemfile );

   if( ( fp = fopen( filename, "r" ) ) != NULL )
   {

      found = TRUE;
      LINK( starsystem, first_starsystem, last_starsystem, next, prev );
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
            bug( "Load_starsystem_file: # not found.", 0 );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "SPACE" ) )
         {
            fread_starsystem( starsystem, fp );
            break;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            char buf[MAX_STRING_LENGTH];

            sprintf( buf, "Load_starsystem_file: bad section: %s.", word );
            bug( buf, 0 );
            break;
         }
      }
      fclose( fp );
   }

   if( !( found ) )
      DISPOSE( starsystem );

   return found;
}

/*
 * Load in all the starsystem files.
 */
void load_space(  )
{
   FILE *fpList;
   const char *filename;
   char starsystemlist[256];
   char buf[MAX_STRING_LENGTH];


   first_starsystem = NULL;
   last_starsystem = NULL;

   log_string( "Loading space..." );

   sprintf( starsystemlist, "%s%s", SPACE_DIR, SPACE_LIST );
   if( ( fpList = fopen( starsystemlist, "r" ) ) == NULL )
   {
      perror( starsystemlist );
      exit( 1 );
   }

   for( ;; )
   {
      filename = feof( fpList ) ? "$" : fread_word( fpList );
      if( filename[0] == '$' )
         break;


      if( !load_starsystem( filename ) )
      {
         sprintf( buf, "Cannot load starsystem file: %s", filename );
         bug( buf, 0 );
      }
   }
   fclose( fpList );
   log_string( " Done starsystems " );
   return;
}

void do_setstarsystem( CHAR_DATA * ch, const char *argument )
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   SPACE_DATA *starsystem;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg2[0] == '\0' || arg1[0] == '\0' )
   {
      send_to_char( "Usage: setstarsystem <starsystem> <field> <values>\r\n", ch );
      send_to_char( "\r\nField being one of:\r\n", ch );
      send_to_char( "name filename xpos ypos,\r\n", ch );
      send_to_char( "star1 s1x s1y s1z gravitys1\r\n", ch );
      send_to_char( "star2 s2x s2y s2z gravitys2\r\n", ch );
      send_to_char( "planet1 p1x p1y p1z gravityp1\r\n", ch );
      send_to_char( "planet2 p2x p2y p2z gravityp2\r\n", ch );
      send_to_char( "planet3 p3x p3y p3z gravityp3\r\n", ch );
      send_to_char( "location1a location1b location1c doc1a doc1b doc1c\r\n", ch );
      send_to_char( "location2a location2b location2c doc2a doc2b doc2c\r\n", ch );
      send_to_char( "location3a location3b location3c doc3a doc3b doc3c\r\n", ch );
      send_to_char( "", ch );
      return;
   }

   starsystem = starsystem_from_name( arg1 );
   if( !starsystem )
   {
      send_to_char( "No such starsystem.\r\n", ch );
      return;
   }


   if( !str_cmp( arg2, "doc1a" ) )
   {
      starsystem->doc1a = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "doc1b" ) )
   {
      starsystem->doc1b = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "doc1c" ) )
   {
      starsystem->doc1c = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "doc2a" ) )
   {
      starsystem->doc2a = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "doc2b" ) )
   {
      starsystem->doc2b = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "doc2c" ) )
   {
      starsystem->doc2c = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "doc3a" ) )
   {
      starsystem->doc3a = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "doc3b" ) )
   {
      starsystem->doc3b = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "doc3c" ) )
   {
      starsystem->doc3c = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "s1x" ) )
   {
      starsystem->s1x = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "s1y" ) )
   {
      starsystem->s1y = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "s1z" ) )
   {
      starsystem->s1z = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "s2x" ) )
   {
      starsystem->s2x = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "s2y" ) )
   {
      starsystem->s2y = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "s2z" ) )
   {
      starsystem->s2z = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "p1x" ) )
   {
      starsystem->p1x = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "p1y" ) )
   {
      starsystem->p1y = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "p1z" ) )
   {
      starsystem->p1z = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "p2x" ) )
   {
      starsystem->p2x = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "p2y" ) )
   {
      starsystem->p2y = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "p2z" ) )
   {
      starsystem->p2z = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "p3x" ) )
   {
      starsystem->p3x = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "p3y" ) )
   {
      starsystem->p3y = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "p3z" ) )
   {
      starsystem->p3z = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "xpos" ) )
   {
      starsystem->xpos = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "ypos" ) )
   {
      starsystem->ypos = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "gravitys1" ) )
   {
      starsystem->gravitys1 = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "gravitys2" ) )
   {
      starsystem->gravitys2 = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "gravityp1" ) )
   {
      starsystem->gravityp1 = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "gravityp2" ) )
   {
      starsystem->gravityp2 = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "gravityp3" ) )
   {
      starsystem->gravityp3 = atoi( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      STRFREE( starsystem->name );
      starsystem->name = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "star1" ) )
   {
      STRFREE( starsystem->star1 );
      starsystem->star1 = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "star2" ) )
   {
      STRFREE( starsystem->star2 );
      starsystem->star2 = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "planet1" ) )
   {
      STRFREE( starsystem->planet1 );
      starsystem->planet1 = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "planet2" ) )
   {
      STRFREE( starsystem->planet2 );
      starsystem->planet2 = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "planet3" ) )
   {
      STRFREE( starsystem->planet3 );
      starsystem->planet3 = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "location1a" ) )
   {
      STRFREE( starsystem->location1a );
      starsystem->location1a = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "location1b" ) )
   {
      STRFREE( starsystem->location1b );
      starsystem->location1b = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "location1c" ) )
   {
      STRFREE( starsystem->location1c );
      starsystem->location1c = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "location2a" ) )
   {
      STRFREE( starsystem->location2a );
      starsystem->location2a = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "location2b" ) )
   {
      STRFREE( starsystem->location2a );
      starsystem->location2b = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "location2c" ) )
   {
      STRFREE( starsystem->location2c );
      starsystem->location2c = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }

   if( !str_cmp( arg2, "location3a" ) )
   {
      STRFREE( starsystem->location3a );
      starsystem->location3a = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "location3b" ) )
   {
      STRFREE( starsystem->location3b );
      starsystem->location3b = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }
   if( !str_cmp( arg2, "location3c" ) )
   {
      STRFREE( starsystem->location3c );
      starsystem->location3c = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_starsystem( starsystem );
      return;
   }


   do_setstarsystem( ch, "" );
   return;
}

void showstarsystem( CHAR_DATA * ch, SPACE_DATA * starsystem )
{
   ch_printf( ch, "Starsystem:%s     Filename: %s    Xpos: %d   Ypos: %d\r\n",
              starsystem->name, starsystem->filename, starsystem->xpos, starsystem->ypos );
   ch_printf( ch, "Star1: %s   Gravity: %d   Coordinates: %d %d %d\r\n",
              starsystem->star1, starsystem->gravitys1, starsystem->s1x, starsystem->s1y, starsystem->s1z );
   ch_printf( ch, "Star2: %s   Gravity: %d   Coordinates: %d %d %d\r\n",
              starsystem->star2, starsystem->gravitys2, starsystem->s2x, starsystem->s2y, starsystem->s2z );
   ch_printf( ch, "Planet1: %s   Gravity: %d   Coordinates: %d %d %d\r\n",
              starsystem->planet1, starsystem->gravityp1, starsystem->p1x, starsystem->p1y, starsystem->p1z );
   ch_printf( ch, "     Doc1a: %5d (%s)\r\n", starsystem->doc1a, starsystem->location1a );
   ch_printf( ch, "     Doc1b: %5d (%s)\r\n", starsystem->doc1b, starsystem->location1b );
   ch_printf( ch, "     Doc1c: %5d (%s)\r\n", starsystem->doc1c, starsystem->location1c );
   ch_printf( ch, "Planet2: %s   Gravity: %d   Coordinates: %d %d %d\r\n",
              starsystem->planet2, starsystem->gravityp2, starsystem->p2x, starsystem->p2y, starsystem->p2z );
   ch_printf( ch, "     Doc2a: %5d (%s)\r\n", starsystem->doc2a, starsystem->location2a );
   ch_printf( ch, "     Doc2b: %5d (%s)\r\n", starsystem->doc2b, starsystem->location2b );
   ch_printf( ch, "     Doc2c: %5d (%s)\r\n", starsystem->doc2c, starsystem->location2c );
   ch_printf( ch, "Planet3: %s   Gravity: %d   Coordinates: %d %d %d\r\n",
              starsystem->planet3, starsystem->gravityp3, starsystem->p3x, starsystem->p3y, starsystem->p3z );
   ch_printf( ch, "     Doc3a: %5d (%s)\r\n", starsystem->doc3a, starsystem->location3a );
   ch_printf( ch, "     Doc3b: %5d (%s)\r\n", starsystem->doc3b, starsystem->location3b );
   ch_printf( ch, "     Doc3c: %5d (%s)\r\n", starsystem->doc3c, starsystem->location3c );
   return;
}

void do_showstarsystem( CHAR_DATA * ch, const char *argument )
{
   SPACE_DATA *starsystem;

   starsystem = starsystem_from_name( argument );

   if( starsystem == NULL )
      send_to_char( "&RNo such starsystem.\r\n", ch );
   else
      showstarsystem( ch, starsystem );

}

void do_makestarsystem( CHAR_DATA * ch, const char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   char filename[256];
   SPACE_DATA *starsystem;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: makestarsystem <starsystem name>\r\n", ch );
      return;
   }


   CREATE( starsystem, SPACE_DATA, 1 );
   LINK( starsystem, first_starsystem, last_starsystem, next, prev );

   starsystem->name = STRALLOC( argument );

   starsystem->location1a = STRALLOC( "" );
   starsystem->location2a = STRALLOC( "" );
   starsystem->location3a = STRALLOC( "" );
   starsystem->location1b = STRALLOC( "" );
   starsystem->location2b = STRALLOC( "" );
   starsystem->location3b = STRALLOC( "" );
   starsystem->location1c = STRALLOC( "" );
   starsystem->location2c = STRALLOC( "" );
   starsystem->location3c = STRALLOC( "" );
   starsystem->planet1 = STRALLOC( "" );
   starsystem->planet2 = STRALLOC( "" );
   starsystem->planet3 = STRALLOC( "" );
   starsystem->star1 = STRALLOC( "" );
   starsystem->star2 = STRALLOC( "" );

   argument = one_argument( argument, arg );
   sprintf( filename, "%s.system", strlower( arg ) );
   starsystem->filename = str_dup( filename );
   save_starsystem( starsystem );
   write_starsystem_list(  );
}

void do_starsystems( CHAR_DATA * ch, const char *argument )
{
   SPACE_DATA *starsystem;
   int count = 0;

   for( starsystem = first_starsystem; starsystem; starsystem = starsystem->next )
   {
      set_char_color( AT_NOTE, ch );
      ch_printf( ch, "%s\r\n", starsystem->name );
      count++;
   }

   if( !count )
   {
      send_to_char( "There are no starsystems currently formed.\r\n", ch );
      return;
   }
}

void echo_to_ship( int color, SHIP_DATA * ship, const char *argument )
{
   int room;

   for( room = ship->firstroom; room <= ship->lastroom; room++ )
   {
      echo_to_room( color, get_room_index( room ), argument );
   }

}

void sound_to_ship( SHIP_DATA * ship, const char *argument )
{
   int roomnum;
   ROOM_INDEX_DATA *room;
   CHAR_DATA *vic;

   for( roomnum = ship->firstroom; roomnum <= ship->lastroom; roomnum++ )
   {
      room = get_room_index( roomnum );
      if( room == NULL )
         continue;

      for( vic = room->first_person; vic; vic = vic->next_in_room )
      {
         if( !IS_NPC( vic ) && IS_SET( vic->act, PLR_SOUND ) )
            send_to_char( argument, vic );
      }
   }

}

void echo_to_cockpit( int color, SHIP_DATA * ship, const char *argument )
{
   int room;

   for( room = ship->firstroom; room <= ship->lastroom; room++ )
   {
      if( room == ship->cockpit || room == ship->navseat
          || room == ship->pilotseat || room == ship->coseat
          || room == ship->gunseat || room == ship->engineroom || room == ship->turret1 || room == ship->turret2 )
         echo_to_room( color, get_room_index( room ), argument );
   }

}

void echo_to_system( int color, SHIP_DATA * ship, const char *argument,
		     SHIP_DATA * ignore )
{
   SHIP_DATA *target;

   if( !ship->starsystem )
      return;

   for( target = ship->starsystem->first_ship; target; target = target->next_in_starsystem )
   {
      if( target != ship && target != ignore )
         echo_to_cockpit( color, target, argument );
   }

}

bool is_facing( SHIP_DATA * ship, SHIP_DATA * target )
{
   float dy, dx, dz, hx, hy, hz;
   float cosofa;

   hx = ship->hx;
   hy = ship->hy;
   hz = ship->hz;

   dx = target->vx - ship->vx;
   dy = target->vy - ship->vy;
   dz = target->vz - ship->vz;

   cosofa = ( hx * dx + hy * dy + hz * dz ) / ( sqrt( hx * hx + hy * hy + hz * hz ) + sqrt( dx * dx + dy * dy + dz * dz ) );

   if( cosofa > 0.75 )
      return TRUE;

   return FALSE;
}


long int get_ship_value( SHIP_DATA * ship )
{
   long int price;

   if( ship->ship_class == FIGHTER_SHIP )
      price = 5000;
   else if( ship->ship_class == MIDSIZE_SHIP )
      price = 50000;
   else if( ship->ship_class == CAPITAL_SHIP )
      price = 500000;
   else
      price = 2000;

   if( ship->ship_class <= CAPITAL_SHIP )
      price += ( ship->manuever * 100 * ( 1 + ship->ship_class ) );

   price += ( ship->tractorbeam * 100 );
   price += ( ship->realspeed * 10 );
   price += ( ship->astro_array * 5 );
   price += ( 5 * ship->maxhull );
   price += ( 2 * ship->maxenergy );
   price += ( 100 * ship->maxchaff );

   if( ship->maxenergy > 5000 )
      price += ( ( ship->maxenergy - 5000 ) * 20 );

   if( ship->maxenergy > 10000 )
      price += ( ( ship->maxenergy - 10000 ) * 50 );

   if( ship->maxhull > 1000 )
      price += ( ( ship->maxhull - 1000 ) * 10 );

   if( ship->maxhull > 10000 )
      price += ( ( ship->maxhull - 10000 ) * 20 );

   if( ship->maxshield > 200 )
      price += ( ( ship->maxshield - 200 ) * 50 );

   if( ship->maxshield > 1000 )
      price += ( ( ship->maxshield - 1000 ) * 100 );

   if( ship->realspeed > 100 )
      price += ( ( ship->realspeed - 100 ) * 500 );

   if( ship->lasers > 5 )
      price += ( ( ship->lasers - 5 ) * 500 );

   if( ship->maxshield )
      price += ( 1000 + 10 * ship->maxshield );

   if( ship->lasers )
      price += ( 500 + 500 * ship->lasers );

   if( ship->maxmissiles )
      price += ( 1000 + 100 * ship->maxmissiles );
   if( ship->maxrockets )
      price += ( 2000 + 200 * ship->maxmissiles );
   if( ship->maxtorpedos )
      price += ( 1500 + 150 * ship->maxmissiles );

   if( ship->missiles )
      price += ( 250 * ship->missiles );
   else if( ship->torpedos )
      price += ( 500 * ship->torpedos );
   else if( ship->rockets )
      price += ( 1000 * ship->rockets );

   if( ship->turret1 )
      price += 5000;

   if( ship->turret2 )
      price += 5000;

   if( ship->hyperspeed )
      price += ( 1000 + ship->hyperspeed * 10 );

   if( ship->hanger )
      price += ( ship->ship_class == MIDSIZE_SHIP ? 50000 : 100000 );

   price = ( long )( price * 1.5 );

   return price;

}

void write_ship_list(  )
{
   SHIP_DATA *tship;
   FILE *fpout;
   char filename[256];

   sprintf( filename, "%s%s", SHIP_DIR, SHIP_LIST );
   fpout = fopen( filename, "w" );
   if( !fpout )
   {
      bug( "FATAL: cannot open ship.lst for writing!\r\n", 0 );
      return;
   }
   for( tship = first_ship; tship; tship = tship->next )
      fprintf( fpout, "%s\n", tship->filename );
   fprintf( fpout, "$\n" );
   fclose( fpout );
}

SHIP_DATA *ship_in_room( ROOM_INDEX_DATA * room, const char *name )
{
   SHIP_DATA *ship;

   if( !room )
      return NULL;

   for( ship = room->first_ship; ship; ship = ship->next_in_room )
      if( !str_cmp( name, ship->name ) )
         return ship;

   for( ship = room->first_ship; ship; ship = ship->next_in_room )
      if( nifty_is_name_prefix( name, ship->name ) )
         return ship;

   return NULL;
}

/*
 * Get pointer to ship structure from ship name.
 */
SHIP_DATA *get_ship( const char *name )
{
   SHIP_DATA *ship;

   for( ship = first_ship; ship; ship = ship->next )
      if( !str_cmp( name, ship->name ) )
         return ship;

   for( ship = first_ship; ship; ship = ship->next )
      if( nifty_is_name_prefix( name, ship->name ) )
         return ship;

   return NULL;
}

/*
 * Checks if ships in a starsystem and returns poiner if it is.
 */
SHIP_DATA *get_ship_here( const char *name, SPACE_DATA * starsystem )
{
   SHIP_DATA *ship;

   if( starsystem == NULL )
      return NULL;

   for( ship = starsystem->first_ship; ship; ship = ship->next_in_starsystem )
      if( !str_cmp( name, ship->name ) )
         return ship;

   for( ship = starsystem->first_ship; ship; ship = ship->next_in_starsystem )
      if( nifty_is_name_prefix( name, ship->name ) )
         return ship;

   return NULL;
}


/*
 * Get pointer to ship structure from ship name.
 */
SHIP_DATA *ship_from_pilot( const char *name )
{
   SHIP_DATA *ship;

   for( ship = first_ship; ship; ship = ship->next )
      if( !str_cmp( name, ship->pilot ) )
         return ship;
   if( !str_cmp( name, ship->copilot ) )
      return ship;
   if( !str_cmp( name, ship->owner ) )
      return ship;
   return NULL;
}


/*
 * Get pointer to ship structure from cockpit, turret, or entrance ramp vnum.
 */

SHIP_DATA *ship_from_cockpit( int vnum )
{
   SHIP_DATA *ship;

   for( ship = first_ship; ship; ship = ship->next )
      if( vnum == ship->cockpit || vnum == ship->turret1 || vnum == ship->turret2
          || vnum == ship->pilotseat || vnum == ship->coseat || vnum == ship->navseat
          || vnum == ship->gunseat || vnum == ship->engineroom )
         return ship;
   return NULL;
}

SHIP_DATA *ship_from_pilotseat( int vnum )
{
   SHIP_DATA *ship;

   for( ship = first_ship; ship; ship = ship->next )
      if( vnum == ship->pilotseat )
         return ship;
   return NULL;
}

SHIP_DATA *ship_from_coseat( int vnum )
{
   SHIP_DATA *ship;

   for( ship = first_ship; ship; ship = ship->next )
      if( vnum == ship->coseat )
         return ship;
   return NULL;
}

SHIP_DATA *ship_from_navseat( int vnum )
{
   SHIP_DATA *ship;

   for( ship = first_ship; ship; ship = ship->next )
      if( vnum == ship->navseat )
         return ship;
   return NULL;
}

SHIP_DATA *ship_from_gunseat( int vnum )
{
   SHIP_DATA *ship;

   for( ship = first_ship; ship; ship = ship->next )
      if( vnum == ship->gunseat )
         return ship;
   return NULL;
}

SHIP_DATA *ship_from_engine( int vnum )
{
   SHIP_DATA *ship;

   for( ship = first_ship; ship; ship = ship->next )
   {
      if( ship->engineroom )
      {
         if( vnum == ship->engineroom )
            return ship;
      }
      else
      {
         if( vnum == ship->cockpit )
            return ship;
      }
   }

   return NULL;
}



SHIP_DATA *ship_from_turret( int vnum )
{
   SHIP_DATA *ship;

   for( ship = first_ship; ship; ship = ship->next )
      if( vnum == ship->gunseat || vnum == ship->turret1 || vnum == ship->turret2 )
         return ship;
   return NULL;
}

SHIP_DATA *ship_from_entrance( int vnum )
{
   SHIP_DATA *ship;

   for( ship = first_ship; ship; ship = ship->next )
      if( vnum == ship->entrance )
         return ship;
   return NULL;
}

SHIP_DATA *ship_from_hanger( int vnum )
{
   SHIP_DATA *ship;

   for( ship = first_ship; ship; ship = ship->next )
      if( vnum == ship->hanger )
         return ship;
   return NULL;
}


void save_ship( SHIP_DATA * ship )
{
   FILE *fp;
   char filename[256];
   char buf[MAX_STRING_LENGTH];

   if( !ship )
   {
      bug( "save_ship: null ship pointer!", 0 );
      return;
   }

   if( !ship->filename || ship->filename[0] == '\0' )
   {
      sprintf( buf, "save_ship: %s has no filename", ship->name );
      bug( buf, 0 );
      return;
   }

   sprintf( filename, "%s%s", SHIP_DIR, ship->filename );

   if( ( fp = fopen( filename, "w" ) ) == NULL )
   {
      bug( "save_ship: fopen", 0 );
      perror( filename );
   }
   else
   {
      fprintf( fp, "#SHIP\n" );
      fprintf( fp, "Name         %s~\n", ship->name );
      fprintf( fp, "Filename     %s~\n", ship->filename );
      fprintf( fp, "Description  %s~\n", ship->description );
      fprintf( fp, "Owner        %s~\n", ship->owner );
      fprintf( fp, "Pilot        %s~\n", ship->pilot );
      fprintf( fp, "Copilot      %s~\n", ship->copilot );
      fprintf( fp, "Class        %d\n", ship->ship_class );
      fprintf( fp, "Tractorbeam  %d\n", ship->tractorbeam );
      fprintf( fp, "Shipyard     %d\n", ship->shipyard );
      fprintf( fp, "Hanger       %d\n", ship->hanger );
      fprintf( fp, "Turret1      %d\n", ship->turret1 );
      fprintf( fp, "Turret2      %d\n", ship->turret2 );
      fprintf( fp, "Statet0      %d\n", ship->statet0 );
      fprintf( fp, "Statet1      %d\n", ship->statet1 );
      fprintf( fp, "Statet2      %d\n", ship->statet2 );
      fprintf( fp, "Lasers       %d\n", ship->lasers );
      fprintf( fp, "Missiles     %d\n", ship->missiles );
      fprintf( fp, "Maxmissiles  %d\n", ship->maxmissiles );
      fprintf( fp, "Rockets     %d\n", ship->rockets );
      fprintf( fp, "Maxrockets  %d\n", ship->maxrockets );
      fprintf( fp, "Torpedos     %d\n", ship->torpedos );
      fprintf( fp, "Maxtorpedos  %d\n", ship->maxtorpedos );
      fprintf( fp, "Lastdoc      %d\n", ship->lastdoc );
      fprintf( fp, "Firstroom    %d\n", ship->firstroom );
      fprintf( fp, "Lastroom     %d\n", ship->lastroom );
      fprintf( fp, "Shield       %d\n", ship->shield );
      fprintf( fp, "Maxshield    %d\n", ship->maxshield );
      fprintf( fp, "Hull         %d\n", ship->hull );
      fprintf( fp, "Maxhull      %d\n", ship->maxhull );
      fprintf( fp, "Maxenergy    %d\n", ship->maxenergy );
      fprintf( fp, "Hyperspeed   %d\n", ship->hyperspeed );
      fprintf( fp, "Comm         %d\n", ship->comm );
      fprintf( fp, "Chaff        %d\n", ship->chaff );
      fprintf( fp, "Maxchaff     %d\n", ship->maxchaff );
      fprintf( fp, "Sensor       %d\n", ship->sensor );
      fprintf( fp, "Astro_array  %d\n", ship->astro_array );
      fprintf( fp, "Realspeed    %d\n", ship->realspeed );
      fprintf( fp, "Type         %d\n", ship->type );
      fprintf( fp, "Cockpit      %d\n", ship->cockpit );
      fprintf( fp, "Coseat       %d\n", ship->coseat );
      fprintf( fp, "Pilotseat    %d\n", ship->pilotseat );
      fprintf( fp, "Gunseat      %d\n", ship->gunseat );
      fprintf( fp, "Navseat      %d\n", ship->navseat );
      fprintf( fp, "Engineroom   %d\n", ship->engineroom );
      fprintf( fp, "Entrance     %d\n", ship->entrance );
      fprintf( fp, "Shipstate    %d\n", ship->shipstate );
      fprintf( fp, "Missilestate %d\n", ship->missilestate );
      fprintf( fp, "Energy       %d\n", ship->energy );
      fprintf( fp, "Manuever     %d\n", ship->manuever );
      fprintf( fp, "Home         %s~\n", ship->home );
      fprintf( fp, "End\n\n" );
      fprintf( fp, "#END\n" );
      fclose( fp );
      fp = NULL;
   }
   return;
}

/*
 * Read in actual ship data.
 */
void fread_ship( SHIP_DATA * ship, FILE * fp )
{
   char buf[MAX_STRING_LENGTH];
   const char *word;
   bool fMatch;
   int dummy_number;


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
            KEY( "Astro_array", ship->astro_array, fread_number( fp ) );
            break;

         case 'C':
            KEY( "Cockpit", ship->cockpit, fread_number( fp ) );
            KEY( "Coseat", ship->coseat, fread_number( fp ) );
            KEY( "Class", ship->ship_class, fread_number( fp ) );
            KEY( "Copilot", ship->copilot, fread_string( fp ) );
            KEY( "Comm", ship->comm, fread_number( fp ) );
            KEY( "Chaff", ship->chaff, fread_number( fp ) );
            break;


         case 'D':
            KEY( "Description", ship->description, fread_string( fp ) );
            break;

         case 'E':
            KEY( "Engineroom", ship->engineroom, fread_number( fp ) );
            KEY( "Entrance", ship->entrance, fread_number( fp ) );
            KEY( "Energy", ship->energy, fread_number( fp ) );
            if( !str_cmp( word, "End" ) )
            {
               if( !ship->home )
                  ship->home = STRALLOC( "" );
               if( !ship->name )
                  ship->name = STRALLOC( "" );
               if( !ship->owner )
                  ship->owner = STRALLOC( "" );
               if( !ship->description )
                  ship->description = STRALLOC( "" );
               if( !ship->copilot )
                  ship->copilot = STRALLOC( "" );
               if( !ship->pilot )
                  ship->pilot = STRALLOC( "" );
               if( ship->shipstate != SHIP_DISABLED )
                  ship->shipstate = SHIP_DOCKED;
               if( ship->statet0 != LASER_DAMAGED )
                  ship->statet0 = LASER_READY;
               if( ship->statet1 != LASER_DAMAGED )
                  ship->statet1 = LASER_READY;
               if( ship->statet2 != LASER_DAMAGED )
                  ship->statet2 = LASER_READY;
               if( ship->missilestate != MISSILE_DAMAGED )
                  ship->missilestate = MISSILE_READY;
               if( ship->shipyard <= 0 )
                  ship->shipyard = ROOM_LIMBO_SHIPYARD;
               if( ship->lastdoc <= 0 )
                  ship->lastdoc = ship->shipyard;
               ship->bayopen = TRUE;
               ship->autopilot = FALSE;
               ship->hatchopen = FALSE;
               if( ship->navseat <= 0 )
                  ship->navseat = ship->cockpit;
               if( ship->gunseat <= 0 )
                  ship->gunseat = ship->cockpit;
               if( ship->coseat <= 0 )
                  ship->coseat = ship->cockpit;
               if( ship->pilotseat <= 0 )
                  ship->pilotseat = ship->cockpit;
               if( ship->missiletype == 1 )
               {
                  ship->torpedos = ship->missiles; /* for back compatability */
                  ship->missiles = 0;
               }
               ship->starsystem = NULL;
               ship->energy = ship->maxenergy;
               ship->hull = ship->maxhull;
               ship->in_room = NULL;
               ship->next_in_room = NULL;
               ship->prev_in_room = NULL;

               return;
            }
            break;

         case 'F':
            KEY( "Filename", ship->filename, fread_string_nohash( fp ) );
            KEY( "Firstroom", ship->firstroom, fread_number( fp ) );
            break;

         case 'G':
            KEY( "Gunseat", ship->gunseat, fread_number( fp ) );
            break;

         case 'H':
            KEY( "Home", ship->home, fread_string( fp ) );
            KEY( "Hyperspeed", ship->hyperspeed, fread_number( fp ) );
            KEY( "Hull", ship->hull, fread_number( fp ) );
            KEY( "Hanger", ship->hanger, fread_number( fp ) );
            break;

         case 'L':
            KEY( "Laserstr", ship->lasers, ( short )( fread_number( fp ) / 10 ) );
            KEY( "Lasers", ship->lasers, fread_number( fp ) );
            KEY( "Lastdoc", ship->lastdoc, fread_number( fp ) );
            KEY( "Lastroom", ship->lastroom, fread_number( fp ) );
            break;

         case 'M':
            KEY( "Manuever", ship->manuever, fread_number( fp ) );
            KEY( "Maxmissiles", ship->maxmissiles, fread_number( fp ) );
            KEY( "Maxtorpedos", ship->maxtorpedos, fread_number( fp ) );
            KEY( "Maxrockets", ship->maxrockets, fread_number( fp ) );
            KEY( "Missiles", ship->missiles, fread_number( fp ) );
            KEY( "Missiletype", ship->missiletype, fread_number( fp ) );
            KEY( "Maxshield", ship->maxshield, fread_number( fp ) );
            KEY( "Maxenergy", ship->maxenergy, fread_number( fp ) );
            KEY( "Missilestate", ship->missilestate, fread_number( fp ) );
            KEY( "Maxhull", ship->maxhull, fread_number( fp ) );
            KEY( "Maxchaff", ship->maxchaff, fread_number( fp ) );
            break;

         case 'N':
            KEY( "Name", ship->name, fread_string( fp ) );
            KEY( "Navseat", ship->navseat, fread_number( fp ) );
            break;

         case 'O':
            KEY( "Owner", ship->owner, fread_string( fp ) );
            KEY( "Objectnum", dummy_number, fread_number( fp ) );
            break;

         case 'P':
            KEY( "Pilot", ship->pilot, fread_string( fp ) );
            KEY( "Pilotseat", ship->pilotseat, fread_number( fp ) );
            break;

         case 'R':
            KEY( "Realspeed", ship->realspeed, fread_number( fp ) );
            KEY( "Rockets", ship->rockets, fread_number( fp ) );
            break;

         case 'S':
            KEY( "Shipyard", ship->shipyard, fread_number( fp ) );
            KEY( "Sensor", ship->sensor, fread_number( fp ) );
            KEY( "Shield", ship->shield, fread_number( fp ) );
            KEY( "Shipstate", ship->shipstate, fread_number( fp ) );
            KEY( "Statet0", ship->statet0, fread_number( fp ) );
            KEY( "Statet1", ship->statet1, fread_number( fp ) );
            KEY( "Statet2", ship->statet2, fread_number( fp ) );
            break;

         case 'T':
            KEY( "Type", ship->type, fread_number( fp ) );
            KEY( "Tractorbeam", ship->tractorbeam, fread_number( fp ) );
            KEY( "Turret1", ship->turret1, fread_number( fp ) );
            KEY( "Turret2", ship->turret2, fread_number( fp ) );
            KEY( "Torpedos", ship->torpedos, fread_number( fp ) );
            break;
      }

      if( !fMatch )
      {
         sprintf( buf, "Fread_ship: no match: %s", word );
         bug( buf, 0 );
      }
   }
}

/*
 * Load a ship file
 */

bool load_ship_file( const char *shipfile )
{
   char filename[256];
   SHIP_DATA *ship;
   FILE *fp;
   bool found;
   ROOM_INDEX_DATA *pRoomIndex;
   CLAN_DATA *clan;

   CREATE( ship, SHIP_DATA, 1 );

   found = FALSE;
   sprintf( filename, "%s%s", SHIP_DIR, shipfile );

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
            bug( "Load_ship_file: # not found.", 0 );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "SHIP" ) )
         {
            fread_ship( ship, fp );
            break;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            char buf[MAX_STRING_LENGTH];

            sprintf( buf, "Load_ship_file: bad section: %s.", word );
            bug( buf, 0 );
            break;
         }
      }
      fclose( fp );
   }
   if( !( found ) )
      DISPOSE( ship );
   else
   {
      LINK( ship, first_ship, last_ship, next, prev );
      if( !str_cmp( "Public", ship->owner ) || ship->type == MOB_SHIP )
      {

	if( ship->ship_class != SHIP_PLATFORM && ship->type != MOB_SHIP && ship->ship_class != CAPITAL_SHIP )
         {
            extract_ship( ship );
            ship_to_room( ship, ship->shipyard );

            ship->location = ship->shipyard;
            ship->lastdoc = ship->shipyard;
            ship->shipstate = SHIP_DOCKED;
         }

         ship->currspeed = 0;
         ship->energy = ship->maxenergy;
         ship->chaff = ship->maxchaff;
         ship->hull = ship->maxhull;
         ship->shield = 0;

         ship->statet1 = LASER_READY;
         ship->statet2 = LASER_READY;
         ship->statet0 = LASER_READY;
         ship->missilestate = LASER_READY;

         ship->currjump = NULL;
         ship->target0 = NULL;
         ship->target1 = NULL;
         ship->target2 = NULL;

         ship->hatchopen = FALSE;
         ship->bayopen = TRUE;

         ship->missiles = ship->maxmissiles;
         ship->torpedos = ship->maxtorpedos;
         ship->rockets = ship->maxrockets;
         ship->autorecharge = FALSE;
         ship->autotrack = FALSE;
         ship->autospeed = FALSE;


      }

      else if( ship->cockpit == ROOM_SHUTTLE_BUS ||
               ship->cockpit == ROOM_SHUTTLE_BUS_2 ||
               ship->cockpit == ROOM_SENATE_SHUTTLE ||
               ship->cockpit == ROOM_CORUSCANT_TURBOCAR || ship->cockpit == ROOM_CORUSCANT_SHUTTLE )
      {
      }
      else if( ( pRoomIndex = get_room_index( ship->lastdoc ) ) != NULL
               && ship->ship_class != CAPITAL_SHIP
	       && ship->ship_class != SHIP_PLATFORM )
	{
         LINK( ship, pRoomIndex->first_ship, pRoomIndex->last_ship, next_in_room, prev_in_room );
         ship->in_room = pRoomIndex;
         ship->location = ship->lastdoc;
      }


      if( ship->ship_class == SHIP_PLATFORM
	  || ship->type == MOB_SHIP || ship->ship_class == CAPITAL_SHIP )
      {
         ship_to_starsystem( ship, starsystem_from_name( ship->home ) );
         ship->vx = number_range( -5000, 5000 );
         ship->vy = number_range( -5000, 5000 );
         ship->vz = number_range( -5000, 5000 );
         ship->hx = 1;
         ship->hy = 1;
         ship->hz = 1;
         ship->shipstate = SHIP_READY;
         ship->autopilot = TRUE;
         ship->autorecharge = TRUE;
         ship->shield = ship->maxshield;
      }

      if( ship->type != MOB_SHIP && ( clan = get_clan( ship->owner ) ) != NULL )
      {
	if( ship->ship_class <= SHIP_PLATFORM )
            clan->spacecraft++;
         else
            clan->vehicles++;
      }

   }

   return found;
}

/*
 * Load in all the ship files.
 */
void load_ships(  )
{
   FILE *fpList;
   const char *filename;
   char shiplist[256];
   char buf[MAX_STRING_LENGTH];


   first_ship = NULL;
   last_ship = NULL;
   first_missile = NULL;
   last_missile = NULL;

   log_string( "Loading ships..." );

   sprintf( shiplist, "%s%s", SHIP_DIR, SHIP_LIST );
   if( ( fpList = fopen( shiplist, "r" ) ) == NULL )
   {
      perror( shiplist );
      exit( 1 );
   }

   for( ;; )
   {

      filename = feof( fpList ) ? "$" : fread_word( fpList );

      if( filename[0] == '$' )
         break;

      if( !load_ship_file( filename ) )
      {
         sprintf( buf, "Cannot load ship file: %s", filename );
         bug( buf, 0 );
      }

   }
   fclose( fpList );
   log_string( " Done ships " );
   return;
}

void resetship( SHIP_DATA * ship )
{
   ship->shipstate = SHIP_READY;

   if( ship->ship_class != SHIP_PLATFORM && ship->type != MOB_SHIP )
   {
      extract_ship( ship );
      ship_to_room( ship, ship->shipyard );

      ship->location = ship->shipyard;
      ship->lastdoc = ship->shipyard;
      ship->shipstate = SHIP_DOCKED;
   }

   if( ship->starsystem )
      ship_from_starsystem( ship, ship->starsystem );

   ship->currspeed = 0;
   ship->energy = ship->maxenergy;
   ship->chaff = ship->maxchaff;
   ship->hull = ship->maxhull;
   ship->shield = 0;

   ship->statet1 = LASER_READY;
   ship->statet2 = LASER_READY;
   ship->statet0 = LASER_READY;
   ship->missilestate = LASER_READY;

   ship->currjump = NULL;
   ship->target0 = NULL;
   ship->target1 = NULL;
   ship->target2 = NULL;

   ship->hatchopen = FALSE;
   ship->bayopen = TRUE;

   ship->missiles = ship->maxmissiles;
   ship->torpedos = ship->maxtorpedos;
   ship->rockets = ship->maxrockets;
   ship->autorecharge = FALSE;
   ship->autotrack = FALSE;
   ship->autospeed = FALSE;

   if( str_cmp( "Public", ship->owner ) && ship->type != MOB_SHIP )
   {
      CLAN_DATA *clan;

      if( ship->type != MOB_SHIP && ( clan = get_clan( ship->owner ) ) != NULL )
      {
	if( ship->ship_class <= SHIP_PLATFORM )
	  clan->spacecraft--;
	else
	  clan->vehicles--;
      }

      STRFREE( ship->owner );
      ship->owner = STRALLOC( "" );
      STRFREE( ship->pilot );
      ship->pilot = STRALLOC( "" );
      STRFREE( ship->copilot );
      ship->copilot = STRALLOC( "" );
   }

   if( ship->type == SHIP_REPUBLIC || ( ship->type == MOB_SHIP && !str_cmp( ship->owner, "the new republic" ) ) )
   {
      STRFREE( ship->home );
      ship->home = STRALLOC( "coruscant" );
   }
   else if( ship->type == SHIP_IMPERIAL || ( ship->type == MOB_SHIP && !str_cmp( ship->owner, "the empire" ) ) )
   {
      STRFREE( ship->home );
      ship->home = STRALLOC( "byss" );
   }
   else if( ship->type == SHIP_CIVILIAN )
   {
      STRFREE( ship->home );
      ship->home = STRALLOC( "corperate" );
   }

   save_ship( ship );
}

void do_resetship( CHAR_DATA * ch, const char *argument )
{
   SHIP_DATA *ship;

   ship = get_ship( argument );
   if( ship == NULL )
   {
      send_to_char( "&RNo such ship!", ch );
      return;
   }

   resetship( ship );

   if( ( ship->ship_class == SHIP_PLATFORM || ship->type == MOB_SHIP || ship->ship_class == CAPITAL_SHIP ) && ship->home )
   {
      ship_to_starsystem( ship, starsystem_from_name( ship->home ) );
      ship->vx = number_range( -5000, 5000 );
      ship->vy = number_range( -5000, 5000 );
      ship->vz = number_range( -5000, 5000 );
      ship->shipstate = SHIP_READY;
      ship->autopilot = TRUE;
      ship->autorecharge = TRUE;
      ship->shield = ship->maxshield;
   }

}

void do_setship( CHAR_DATA * ch, const char *argument )
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   SHIP_DATA *ship;
   int tempnum;
   ROOM_INDEX_DATA *roomindex;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' || arg2[0] == '\0' || arg1[0] == '\0' )
   {
      send_to_char( "Usage: setship <ship> <field> <values>\r\n", ch );
      send_to_char( "\r\nField being one of:\r\n", ch );
      send_to_char( "filename name owner copilot pilot description home\r\n", ch );
      send_to_char( "cockpit entrance turret1 turret2 hanger\r\n", ch );
      send_to_char( "engineroom firstroom lastroom shipyard\r\n", ch );
      send_to_char( "manuever speed hyperspeed tractorbeam\r\n", ch );
      send_to_char( "lasers missiles shield hull energy chaff\r\n", ch );
      send_to_char( "comm sensor astroarray class torpedos\r\n", ch );
      send_to_char( "pilotseat coseat gunseat navseat rockets\r\n", ch );
      return;
   }

   ship = get_ship( arg1 );
   if( !ship )
   {
      send_to_char( "No such ship.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "owner" ) )
   {
      CLAN_DATA *clan;
      if( ship->type != MOB_SHIP && ( clan = get_clan( ship->owner ) ) != NULL )
      {
	if( ship->ship_class <= SHIP_PLATFORM )
	  clan->spacecraft--;
	else
	  clan->vehicles--;
      }
      STRFREE( ship->owner );
      ship->owner = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      if( ship->type != MOB_SHIP && ( clan = get_clan( ship->owner ) ) != NULL )
      {
	if( ship->ship_class <= SHIP_PLATFORM )
	  clan->spacecraft++;
	else
	  clan->vehicles++;
      }
      return;
   }

   if( !str_cmp( arg2, "home" ) )
   {
      STRFREE( ship->home );
      ship->home = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "pilot" ) )
   {
      STRFREE( ship->pilot );
      ship->pilot = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "copilot" ) )
   {
      STRFREE( ship->copilot );
      ship->copilot = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "firstroom" ) )
   {
      tempnum = atoi( argument );
      roomindex = get_room_index( tempnum );
      if( roomindex == NULL )
      {
         send_to_char( "That room doesn't exist.\r\n", ch );
         return;
      }
      ship->firstroom = tempnum;
      ship->lastroom = tempnum;
      ship->cockpit = tempnum;
      ship->coseat = tempnum;
      ship->pilotseat = tempnum;
      ship->gunseat = tempnum;
      ship->navseat = tempnum;
      ship->entrance = tempnum;
      ship->turret1 = 0;
      ship->turret2 = 0;
      ship->hanger = 0;
      send_to_char( "You will now need to set the other rooms in the ship.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "lastroom" ) )
   {
      tempnum = atoi( argument );
      roomindex = get_room_index( tempnum );
      if( roomindex == NULL )
      {
         send_to_char( "That room doesn't exist.\r\n", ch );
         return;
      }
      if( tempnum < ship->firstroom )
      {
         send_to_char( "The last room on a ship must be greater than or equal to the first room.\r\n", ch );
         return;
      }
      if( ship->ship_class == FIGHTER_SHIP
	  && ( tempnum - ship->firstroom ) > 5 )
      {
         send_to_char( "Starfighters may have up to 5 rooms only.\r\n", ch );
         return;
      }
      if( ship->ship_class == MIDSIZE_SHIP
	  && ( tempnum - ship->firstroom ) > 25 )
      {
         send_to_char( "Midships may have up to 25 rooms only.\r\n", ch );
         return;
      }
      if( ship->ship_class == CAPITAL_SHIP
	  && ( tempnum - ship->firstroom ) > 100 )
      {
         send_to_char( "Capital Ships may have up to 100 rooms only.\r\n", ch );
         return;
      }
      ship->lastroom = tempnum;
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "cockpit" ) )
   {
      tempnum = atoi( argument );
      roomindex = get_room_index( tempnum );
      if( roomindex == NULL )
      {
         send_to_char( "That room doesn't exist.\r\n", ch );
         return;
      }
      if( tempnum < ship->firstroom || tempnum > ship->lastroom )
      {
         send_to_char( "That room number is not in that ship .. \r\nIt must be between Firstroom and Lastroom.\r\n", ch );
         return;
      }
      if( tempnum == ship->turret1 || tempnum == ship->turret2 || tempnum == ship->hanger )
      {
         send_to_char( "That room is already being used by another part of the ship\r\n", ch );
         return;
      }
      ship->cockpit = tempnum;
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "pilotseat" ) )
   {
      tempnum = atoi( argument );
      roomindex = get_room_index( tempnum );
      if( roomindex == NULL )
      {
         send_to_char( "That room doesn't exist.\r\n", ch );
         return;
      }
      if( tempnum < ship->firstroom || tempnum > ship->lastroom )
      {
         send_to_char( "That room number is not in that ship .. \r\nIt must be between Firstroom and Lastroom.\r\n", ch );
         return;
      }
      if( tempnum == ship->turret1 || tempnum == ship->turret2 || tempnum == ship->hanger )
      {
         send_to_char( "That room is already being used by another part of the ship\r\n", ch );
         return;
      }
      ship->pilotseat = tempnum;
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }
   if( !str_cmp( arg2, "coseat" ) )
   {
      tempnum = atoi( argument );
      roomindex = get_room_index( tempnum );
      if( roomindex == NULL )
      {
         send_to_char( "That room doesn't exist.\r\n", ch );
         return;
      }
      if( tempnum < ship->firstroom || tempnum > ship->lastroom )
      {
         send_to_char( "That room number is not in that ship .. \r\nIt must be between Firstroom and Lastroom.\r\n", ch );
         return;
      }
      if( tempnum == ship->turret1 || tempnum == ship->turret2 || tempnum == ship->hanger )
      {
         send_to_char( "That room is already being used by another part of the ship\r\n", ch );
         return;
      }
      ship->coseat = tempnum;
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }
   if( !str_cmp( arg2, "navseat" ) )
   {
      tempnum = atoi( argument );
      roomindex = get_room_index( tempnum );
      if( roomindex == NULL )
      {
         send_to_char( "That room doesn't exist.\r\n", ch );
         return;
      }
      if( tempnum < ship->firstroom || tempnum > ship->lastroom )
      {
         send_to_char( "That room number is not in that ship .. \r\nIt must be between Firstroom and Lastroom.\r\n", ch );
         return;
      }
      if( tempnum == ship->turret1 || tempnum == ship->turret2 || tempnum == ship->hanger )
      {
         send_to_char( "That room is already being used by another part of the ship\r\n", ch );
         return;
      }
      ship->navseat = tempnum;
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }
   if( !str_cmp( arg2, "gunseat" ) )
   {
      tempnum = atoi( argument );
      roomindex = get_room_index( tempnum );
      if( roomindex == NULL )
      {
         send_to_char( "That room doesn't exist.\r\n", ch );
         return;
      }
      if( tempnum < ship->firstroom || tempnum > ship->lastroom )
      {
         send_to_char( "That room number is not in that ship .. \r\nIt must be between Firstroom and Lastroom.\r\n", ch );
         return;
      }
      if( tempnum == ship->turret1 || tempnum == ship->turret2 || tempnum == ship->hanger )
      {
         send_to_char( "That room is already being used by another part of the ship\r\n", ch );
         return;
      }
      ship->gunseat = tempnum;
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "entrance" ) )
   {
      tempnum = atoi( argument );
      roomindex = get_room_index( tempnum );
      if( roomindex == NULL )
      {
         send_to_char( "That room doesn't exist.\r\n", ch );
         return;
      }
      if( tempnum < ship->firstroom || tempnum > ship->lastroom )
      {
         send_to_char( "That room number is not in that ship .. \r\nIt must be between Firstroom and Lastroom.\r\n", ch );
         return;
      }
      ship->entrance = tempnum;
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "turret1" ) )
   {
      tempnum = atoi( argument );
      roomindex = get_room_index( tempnum );
      if( roomindex == NULL )
      {
         send_to_char( "That room doesn't exist.\r\n", ch );
         return;
      }
      if( tempnum < ship->firstroom || tempnum > ship->lastroom )
      {
         send_to_char( "That room number is not in that ship .. \r\nIt must be between Firstroom and Lastroom.\r\n", ch );
         return;
      }
      if( ship->ship_class == FIGHTER_SHIP )
      {
         send_to_char( "Starfighters can't have extra laser turrets.\r\n", ch );
         return;
      }
      if( tempnum == ship->cockpit || tempnum == ship->entrance ||
          tempnum == ship->turret2 || tempnum == ship->hanger || tempnum == ship->engineroom )
      {
         send_to_char( "That room is already being used by another part of the ship\r\n", ch );
         return;
      }
      ship->turret1 = tempnum;
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "turret2" ) )
   {
      tempnum = atoi( argument );
      roomindex = get_room_index( tempnum );
      if( roomindex == NULL )
      {
         send_to_char( "That room doesn't exist.\r\n", ch );
         return;
      }
      if( tempnum < ship->firstroom || tempnum > ship->lastroom )
      {
         send_to_char( "That room number is not in that ship .. \r\nIt must be between Firstroom and Lastroom.\r\n", ch );
         return;
      }
      if( ship->ship_class == FIGHTER_SHIP )
      {
         send_to_char( "Starfighters can't have extra laser turrets.\r\n", ch );
         return;
      }
      if( tempnum == ship->cockpit || tempnum == ship->entrance ||
          tempnum == ship->turret1 || tempnum == ship->hanger || tempnum == ship->engineroom )
      {
         send_to_char( "That room is already being used by another part of the ship\r\n", ch );
         return;
      }
      ship->turret2 = tempnum;
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "hanger" ) )
   {
      tempnum = atoi( argument );
      roomindex = get_room_index( tempnum );
      if( roomindex == NULL )
      {
         send_to_char( "That room doesn't exist.\r\n", ch );
         return;
      }
      if( tempnum < ship->firstroom || tempnum > ship->lastroom )
      {
         send_to_char( "That room number is not in that ship .. \r\nIt must be between Firstroom and Lastroom.\r\n", ch );
         return;
      }
      if( tempnum == ship->cockpit || tempnum == ship->entrance ||
          tempnum == ship->turret1 || tempnum == ship->turret2 || tempnum == ship->engineroom )
      {
         send_to_char( "That room is already being used by another part of the ship\r\n", ch );
         return;
      }
      if( ship->ship_class == FIGHTER_SHIP )
      {
         send_to_char( "Starfighters are to small to have hangers for other ships!\r\n", ch );
         return;
      }
      ship->hanger = tempnum;
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "engineroom" ) )
   {
      tempnum = atoi( argument );
      roomindex = get_room_index( tempnum );
      if( roomindex == NULL )
      {
         send_to_char( "That room doesn't exist.\r\n", ch );
         return;
      }
      if( tempnum < ship->firstroom || tempnum > ship->lastroom )
      {
         send_to_char( "That room number is not in that ship .. \r\nIt must be between Firstroom and Lastroom.\r\n", ch );
         return;
      }
      if( tempnum == ship->cockpit || tempnum == ship->entrance ||
          tempnum == ship->turret1 || tempnum == ship->turret2 || tempnum == ship->hanger )
      {
         send_to_char( "That room is already being used by another part of the ship\r\n", ch );
         return;
      }
      ship->engineroom = tempnum;
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "shipyard" ) )
   {
      tempnum = atoi( argument );
      roomindex = get_room_index( tempnum );
      if( roomindex == NULL )
      {
         send_to_char( "That room doesn't exist.", ch );
         return;
      }
      ship->shipyard = tempnum;
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "type" ) )
   {
      if( !str_cmp( argument, "republic" ) )
         ship->type = SHIP_REPUBLIC;
      else if( !str_cmp( argument, "imperial" ) )
         ship->type = SHIP_IMPERIAL;
      else if( !str_cmp( argument, "civilian" ) )
         ship->type = SHIP_CIVILIAN;
      else if( !str_cmp( argument, "mob" ) )
         ship->type = MOB_SHIP;
      else
      {
         send_to_char( "Ship type must be either: republic, imperial, civilian or mob.\r\n", ch );
         return;
      }
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      STRFREE( ship->name );
      ship->name = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "filename" ) )
   {
      DISPOSE( ship->filename );
      ship->filename = str_dup( argument );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      write_ship_list(  );
      return;
   }

   if( !str_cmp( arg2, "desc" ) )
   {
      STRFREE( ship->description );
      ship->description = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "manuever" ) )
   {
      ship->manuever = URANGE( 0, atoi( argument ), 120 );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "lasers" ) )
   {
      ship->lasers = URANGE( 0, atoi( argument ), 10 );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "class" ) )
   {
     ship->ship_class = URANGE( 0, atoi( argument ), 9 );
     send_to_char( "Done.\r\n", ch );
     save_ship( ship );
     return;
   }

   if( !str_cmp( arg2, "missiles" ) )
   {
      ship->maxmissiles = URANGE( 0, atoi( argument ), 255 );
      ship->missiles = URANGE( 0, atoi( argument ), 255 );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "torpedos" ) )
   {
      ship->maxtorpedos = URANGE( 0, atoi( argument ), 255 );
      ship->torpedos = URANGE( 0, atoi( argument ), 255 );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "rockets" ) )
   {
      ship->maxrockets = URANGE( 0, atoi( argument ), 255 );
      ship->rockets = URANGE( 0, atoi( argument ), 255 );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "speed" ) )
   {
      ship->realspeed = URANGE( 0, atoi( argument ), 150 );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "tractorbeam" ) )
   {
      ship->tractorbeam = URANGE( 0, atoi( argument ), 255 );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "hyperspeed" ) )
   {
      ship->hyperspeed = URANGE( 0, atoi( argument ), 255 );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "shield" ) )
   {
      ship->maxshield = URANGE( 0, atoi( argument ), 1000 );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "hull" ) )
   {
      ship->hull = URANGE( 1, atoi( argument ), 20000 );
      ship->maxhull = URANGE( 1, atoi( argument ), 20000 );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "energy" ) )
   {
      ship->energy = URANGE( 1, atoi( argument ), 30000 );
      ship->maxenergy = URANGE( 1, atoi( argument ), 30000 );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "sensor" ) )
   {
      ship->sensor = URANGE( 0, atoi( argument ), 255 );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "astroarray" ) )
   {
      ship->astro_array = URANGE( 0, atoi( argument ), 255 );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "comm" ) )
   {
      ship->comm = URANGE( 0, atoi( argument ), 255 );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( arg2, "chaff" ) )
   {
      ship->chaff = URANGE( 0, atoi( argument ), 25 );
      ship->maxchaff = URANGE( 0, atoi( argument ), 25 );
      send_to_char( "Done.\r\n", ch );
      save_ship( ship );
      return;
   }

   do_setship( ch, "" );
   return;
}

void do_showship( CHAR_DATA * ch, const char *argument )
{
   SHIP_DATA *ship;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      send_to_char( "Usage: showship <ship>\r\n", ch );
      return;
   }

   ship = get_ship( argument );
   if( !ship )
   {
      send_to_char( "No such ship.\r\n", ch );
      return;
   }
   set_char_color( AT_YELLOW, ch );
   ch_printf( ch, "%s %s : %s\r\nFilename: %s\r\n",
              ship->type == SHIP_REPUBLIC ? "New Republic" :
              ( ship->type == SHIP_IMPERIAL ? "Imperial" :
                ( ship->type == SHIP_CIVILIAN ? "Civilian" : "Mob" ) ),
              ship->ship_class == FIGHTER_SHIP ? "Starfighter" :
              ( ship->ship_class == MIDSIZE_SHIP ? "Midship" :
                ( ship->ship_class == CAPITAL_SHIP ? "Capital Ship" :
                  ( ship->ship_class == SHIP_PLATFORM ? "Platform" :
                    ( ship->ship_class == CLOUD_CAR ? "Cloudcar" :
                      ( ship->ship_class == OCEAN_SHIP ? "Boat" :
                        ( ship->ship_class == LAND_SPEEDER ? "Speeder" :
                          ( ship->ship_class == WHEELED ? "Wheeled Transport" :
                            ( ship->ship_class == LAND_CRAWLER ? "Crawler" :
                              ( ship->ship_class == WALKER ? "Walker" : "Unknown" ) ) ) ) ) ) ) ) ), ship->name, ship->filename );
   ch_printf( ch, "Home: %s   Description: %s\r\nOwner: %s   Pilot: %s   Copilot: %s\r\n",
              ship->home, ship->description, ship->owner, ship->pilot, ship->copilot );
   ch_printf( ch, "Firstroom: %d   Lastroom: %d", ship->firstroom, ship->lastroom );
   ch_printf( ch, "Cockpit: %d   Entrance: %d   Hanger: %d  Engineroom: %d\r\n",
              ship->cockpit, ship->entrance, ship->hanger, ship->engineroom );
   ch_printf( ch, "Pilotseat: %d   Coseat: %d   Navseat: %d  Gunseat: %d\r\n",
              ship->pilotseat, ship->coseat, ship->navseat, ship->gunseat );
   ch_printf( ch, "Location: %d   Lastdoc: %d   Shipyard: %d\r\n", ship->location, ship->lastdoc, ship->shipyard );
   ch_printf( ch, "Tractor Beam: %d   Comm: %d   Sensor: %d   Astro Array: %d\r\n",
              ship->tractorbeam, ship->comm, ship->sensor, ship->astro_array );
   ch_printf( ch, "Lasers: %d  Laser Condition: %s\r\n", ship->lasers, ship->statet0 == LASER_DAMAGED ? "Damaged" : "Good" );
   ch_printf( ch, "Turret One: %d  Condition: %s\r\n", ship->turret1, ship->statet1 == LASER_DAMAGED ? "Damaged" : "Good" );
   ch_printf( ch, "Turret Two: %d  Condition: %s\r\n", ship->turret2, ship->statet2 == LASER_DAMAGED ? "Damaged" : "Good" );
   ch_printf( ch, "Missiles: %d/%d  Torpedos: %d/%d  Rockets: %d/%d  Condition: %s\r\n",
              ship->missiles,
              ship->maxmissiles,
              ship->torpedos,
              ship->maxtorpedos,
              ship->rockets, ship->maxrockets, ship->missilestate == MISSILE_DAMAGED ? "Damaged" : "Good" );
   ch_printf( ch, "Hull: %d/%d  Ship Condition: %s\r\n",
              ship->hull, ship->maxhull, ship->shipstate == SHIP_DISABLED ? "Disabled" : "Running" );

   ch_printf( ch, "Shields: %d/%d   Energy(fuel): %d/%d   Chaff: %d/%d\r\n",
              ship->shield, ship->maxshield, ship->energy, ship->maxenergy, ship->chaff, ship->maxchaff );
   ch_printf( ch, "Current Coordinates: %.0f %.0f %.0f\r\n", ship->vx, ship->vy, ship->vz );
   ch_printf( ch, "Current Heading: %.0f %.0f %.0f\r\n", ship->hx, ship->hy, ship->hz );
   ch_printf( ch, "Speed: %d/%d   Hyperspeed: %d\r\n  Manueverability: %d",
              ship->currspeed, ship->realspeed, ship->hyperspeed, ship->manuever );
   return;
}

void do_makeship( CHAR_DATA * ch, const char *argument )
{
   SHIP_DATA *ship;
   char arg[MAX_INPUT_LENGTH];

   argument = one_argument( argument, arg );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: makeship <filename> <ship name>\r\n", ch );
      return;
   }

   CREATE( ship, SHIP_DATA, 1 );
   LINK( ship, first_ship, last_ship, next, prev );

   ship->name = STRALLOC( argument );
   ship->description = STRALLOC( "" );
   ship->owner = STRALLOC( "" );
   ship->copilot = STRALLOC( "" );
   ship->pilot = STRALLOC( "" );
   ship->home = STRALLOC( "" );
   ship->type = SHIP_CIVILIAN;
   ship->starsystem = NULL;
   ship->energy = ship->maxenergy;
   ship->hull = ship->maxhull;
   ship->in_room = NULL;
   ship->next_in_room = NULL;
   ship->prev_in_room = NULL;
   ship->currjump = NULL;
   ship->target0 = NULL;
   ship->target1 = NULL;
   ship->target2 = NULL;

   ship->filename = str_dup( arg );
   save_ship( ship );
   write_ship_list(  );

}

void do_copyship( CHAR_DATA * ch, const char *argument )
{
   SHIP_DATA *ship;
   SHIP_DATA *old;
   char arg[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: copyship <oldshipname> <filename> <newshipname>\r\n", ch );
      return;
   }

   old = get_ship( arg );

   if( !old )
   {
      send_to_char( "Thats not a ship!\r\n", ch );
      return;
   }

   CREATE( ship, SHIP_DATA, 1 );
   LINK( ship, first_ship, last_ship, next, prev );

   ship->name = STRALLOC( argument );
   ship->description = STRALLOC( "" );
   ship->owner = STRALLOC( "" );
   ship->copilot = STRALLOC( "" );
   ship->pilot = STRALLOC( "" );
   ship->home = STRALLOC( "" );
   ship->type = old->type;
   ship->ship_class = old->ship_class;
   ship->lasers = old->lasers;
   ship->maxmissiles = old->maxmissiles;
   ship->maxrockets = old->maxrockets;
   ship->maxtorpedos = old->maxtorpedos;
   ship->maxshield = old->maxshield;
   ship->maxhull = old->maxhull;
   ship->maxenergy = old->maxenergy;
   ship->hyperspeed = old->hyperspeed;
   ship->maxchaff = old->maxchaff;
   ship->realspeed = old->realspeed;
   ship->manuever = old->manuever;
   ship->in_room = NULL;
   ship->next_in_room = NULL;
   ship->prev_in_room = NULL;
   ship->currjump = NULL;
   ship->target0 = NULL;
   ship->target1 = NULL;
   ship->target2 = NULL;

   ship->filename = str_dup( arg2 );
   save_ship( ship );
   write_ship_list(  );
}

void do_ships( CHAR_DATA * ch, const char *argument )
{
   SHIP_DATA *ship;
   int count;

   if( !IS_NPC( ch ) )
   {
      count = 0;
      send_to_char( "&YThe following ships are owned by you or by your organization:\r\n", ch );
      send_to_char( "\r\n&WShip                               Owner\r\n", ch );
      for( ship = first_ship; ship; ship = ship->next )
      {
         if( str_cmp( ship->owner, ch->name ) )
         {
            if( !ch->pcdata || !ch->pcdata->clan || str_cmp( ship->owner, ch->pcdata->clan->name )
                || ship->ship_class > SHIP_PLATFORM )
               continue;
         }

         if( ship->type == MOB_SHIP )
            continue;
         else if( ship->type == SHIP_REPUBLIC )
            set_char_color( AT_BLOOD, ch );
         else if( ship->type == SHIP_IMPERIAL )
            set_char_color( AT_DGREEN, ch );
         else
            set_char_color( AT_BLUE, ch );

         if( ship->in_room )
            ch_printf( ch, "%s (%s) - %s\r\n", ship->name, ship->in_room->name );
         else
            ch_printf( ch, "%s (%s)\r\n", ship->name );

         count++;
      }

      if( !count )
      {
         send_to_char( "There are no ships owned by you.\r\n", ch );
      }

   }


   count = 0;
   send_to_char( "&Y\r\nThe following ships are docked here:\r\n", ch );

   send_to_char( "\r\n&WShip                               Owner          Cost/Rent\r\n", ch );
   for( ship = first_ship; ship; ship = ship->next )
   {
      if( ship->location != ch->in_room->vnum
	  || ship->ship_class > SHIP_PLATFORM )
         continue;

      if( ship->type == MOB_SHIP )
         continue;
      else if( ship->type == SHIP_REPUBLIC )
         set_char_color( AT_BLOOD, ch );
      else if( ship->type == SHIP_IMPERIAL )
         set_char_color( AT_DGREEN, ch );
      else
         set_char_color( AT_BLUE, ch );

      ch_printf( ch, "%-35s %-15s", ship->name, ship->owner );
      if( ship->type == MOB_SHIP || ship->ship_class == SHIP_PLATFORM )
      {
         ch_printf( ch, "\r\n" );
         continue;
      }
      if( !str_cmp( ship->owner, "Public" ) )
      {
         ch_printf( ch, "%ld to rent.\r\n", get_ship_value( ship ) / 100 );
      }
      else if( str_cmp( ship->owner, "" ) )
         ch_printf( ch, "%s", "\r\n" );
      else
         ch_printf( ch, "%ld to buy.\r\n", get_ship_value( ship ) );

      count++;
   }

   if( !count )
   {
      send_to_char( "There are no ships docked here.\r\n", ch );
   }
}

void do_speeders( CHAR_DATA * ch, const char *argument )
{
   SHIP_DATA *ship;
   int count;

   if( !IS_NPC( ch ) )
   {
      count = 0;
      send_to_char( "&YThe following are owned by you or by your organization:\r\n", ch );
      send_to_char( "\r\n&WVehicle                            Owner\r\n", ch );
      for( ship = first_ship; ship; ship = ship->next )
      {
         if( str_cmp( ship->owner, ch->name ) )
         {
            if( !ch->pcdata || !ch->pcdata->clan || str_cmp( ship->owner, ch->pcdata->clan->name )
                || ship->ship_class <= SHIP_PLATFORM )
               continue;
         }
         if( ship->location != ch->in_room->vnum
	     || ship->ship_class <= SHIP_PLATFORM )
            continue;

         if( ship->type == MOB_SHIP )
            continue;
         else if( ship->type == SHIP_REPUBLIC )
            set_char_color( AT_BLOOD, ch );
         else if( ship->type == SHIP_IMPERIAL )
            set_char_color( AT_DGREEN, ch );
         else
            set_char_color( AT_BLUE, ch );

         ch_printf( ch, "%-35s %-15s\r\n", ship->name, ship->owner );

         count++;
      }

      if( !count )
      {
         send_to_char( "There are no land or air vehicles owned by you.\r\n", ch );
      }

   }


   count = 0;
   send_to_char( "&Y\r\nThe following vehicles are parked here:\r\n", ch );

   send_to_char( "\r\n&WVehicle                            Owner          Cost/Rent\r\n", ch );
   for( ship = first_ship; ship; ship = ship->next )
   {
      if( ship->location != ch->in_room->vnum
	  || ship->ship_class <= SHIP_PLATFORM )
         continue;

      if( ship->type == MOB_SHIP )
         continue;
      else if( ship->type == SHIP_REPUBLIC )
         set_char_color( AT_BLOOD, ch );
      else if( ship->type == SHIP_IMPERIAL )
         set_char_color( AT_DGREEN, ch );
      else
         set_char_color( AT_BLUE, ch );


      ch_printf( ch, "%-35s %-15s", ship->name, ship->owner );

      if( !str_cmp( ship->owner, "Public" ) )
      {
         ch_printf( ch, "%ld to rent.\r\n", get_ship_value( ship ) / 100 );
      }
      else if( str_cmp( ship->owner, "" ) )
         ch_printf( ch, "%s", "\r\n" );
      else
         ch_printf( ch, "%ld to buy.\r\n", get_ship_value( ship ) );

      count++;
   }

   if( !count )
   {
      send_to_char( "There are no sea air or land vehicles here.\r\n", ch );
   }
}

void do_allspeeders( CHAR_DATA * ch, const char *argument )
{
   SHIP_DATA *ship;
   int count = 0;

   count = 0;
   send_to_char( "&Y\r\nThe following sea/land/air vehicles are currently formed:\r\n", ch );

   send_to_char( "\r\n&WVehicle                            Owner\r\n", ch );
   for( ship = first_ship; ship; ship = ship->next )
   {
      if( ship->ship_class <= SHIP_PLATFORM )
         continue;

      if( ship->type == MOB_SHIP )
         continue;
      else if( ship->type == SHIP_REPUBLIC )
         set_char_color( AT_BLOOD, ch );
      else if( ship->type == SHIP_IMPERIAL )
         set_char_color( AT_DGREEN, ch );
      else
         set_char_color( AT_BLUE, ch );


      ch_printf( ch, "%-35s %-15s ", ship->name, ship->owner );

      if( !str_cmp( ship->owner, "Public" ) )
      {
         ch_printf( ch, "%ld to rent.\r\n", get_ship_value( ship ) / 100 );
      }
      else if( str_cmp( ship->owner, "" ) )
         ch_printf( ch, "%s", "\r\n" );
      else
         ch_printf( ch, "%ld to buy.\r\n", get_ship_value( ship ) );

      count++;
   }

   if( !count )
   {
      send_to_char( "There are none currently formed.\r\n", ch );
      return;
   }

}

void do_allships( CHAR_DATA * ch, const char *argument )
{
   SHIP_DATA *ship;
   int count = 0;

   count = 0;
   send_to_char( "&Y\r\nThe following ships are currently formed:\r\n", ch );

   send_to_char( "\r\n&WShip                               Owner\r\n", ch );

   if( IS_IMMORTAL( ch ) )
      for( ship = first_ship; ship; ship = ship->next )
         if( ship->type == MOB_SHIP )
            ch_printf( ch, "&w%-35s %-15s\r\n", ship->name, ship->owner );

   for( ship = first_ship; ship; ship = ship->next )
   {
     if( ship->ship_class > SHIP_PLATFORM )
       continue;

      if( ship->type == MOB_SHIP )
         continue;
      else if( ship->type == SHIP_REPUBLIC )
         set_char_color( AT_BLOOD, ch );
      else if( ship->type == SHIP_IMPERIAL )
         set_char_color( AT_DGREEN, ch );
      else
         set_char_color( AT_BLUE, ch );

      ch_printf( ch, "%-35s %-15s ", ship->name, ship->owner );
      if( ship->type == MOB_SHIP || ship->ship_class == SHIP_PLATFORM )
      {
         ch_printf( ch, "\r\n" );
         continue;
      }
      if( !str_cmp( ship->owner, "Public" ) )
      {
         ch_printf( ch, "%ld to rent.\r\n", get_ship_value( ship ) / 100 );
      }
      else if( str_cmp( ship->owner, "" ) )
         ch_printf( ch, "%s", "\r\n" );
      else
         ch_printf( ch, "%ld to buy.\r\n", get_ship_value( ship ) );

      count++;
   }

   if( !count )
   {
      send_to_char( "There are no ships currently formed.\r\n", ch );
      return;
   }

}


void ship_to_starsystem( SHIP_DATA * ship, SPACE_DATA * starsystem )
{
   if( starsystem == NULL )
      return;

   if( ship == NULL )
      return;

   if( starsystem->first_ship == NULL )
      starsystem->first_ship = ship;

   if( starsystem->last_ship )
   {
      starsystem->last_ship->next_in_starsystem = ship;
      ship->prev_in_starsystem = starsystem->last_ship;
   }

   starsystem->last_ship = ship;

   ship->starsystem = starsystem;

}

void new_missile( SHIP_DATA * ship, SHIP_DATA * target, CHAR_DATA * ch, int missiletype )
{
   SPACE_DATA *starsystem;
   MISSILE_DATA *missile;

   if( ship == NULL )
      return;

   if( target == NULL )
      return;

   if( ( starsystem = ship->starsystem ) == NULL )
      return;

   CREATE( missile, MISSILE_DATA, 1 );
   LINK( missile, first_missile, last_missile, next, prev );

   missile->target = target;
   missile->fired_from = ship;
   if( ch )
      missile->fired_by = STRALLOC( ch->name );
   else
      missile->fired_by = STRALLOC( "" );
   missile->missiletype = missiletype;
   missile->age = 0;
   if( missile->missiletype == HEAVY_BOMB )
      missile->speed = 20;
   else if( missile->missiletype == PROTON_TORPEDO )
      missile->speed = 200;
   else if( missile->missiletype == CONCUSSION_MISSILE )
      missile->speed = 300;
   else
      missile->speed = 50;

   missile->mx = ( int ) ship->vx;
   missile->my = ( int ) ship->vy;
   missile->mz = ( int ) ship->vz;

   if( starsystem->first_missile == NULL )
      starsystem->first_missile = missile;

   if( starsystem->last_missile )
   {
      starsystem->last_missile->next_in_starsystem = missile;
      missile->prev_in_starsystem = starsystem->last_missile;
   }

   starsystem->last_missile = missile;

   missile->starsystem = starsystem;

}

void ship_from_starsystem( SHIP_DATA * ship, SPACE_DATA * starsystem )
{

   if( starsystem == NULL )
      return;

   if( ship == NULL )
      return;

   if( starsystem->last_ship == ship )
      starsystem->last_ship = ship->prev_in_starsystem;

   if( starsystem->first_ship == ship )
      starsystem->first_ship = ship->next_in_starsystem;

   if( ship->prev_in_starsystem )
      ship->prev_in_starsystem->next_in_starsystem = ship->next_in_starsystem;

   if( ship->next_in_starsystem )
      ship->next_in_starsystem->prev_in_starsystem = ship->prev_in_starsystem;

   ship->starsystem = NULL;
   ship->next_in_starsystem = NULL;
   ship->prev_in_starsystem = NULL;

}

void extract_missile( MISSILE_DATA * missile )
{
   SPACE_DATA *starsystem;

   if( missile == NULL )
      return;

   if( ( starsystem = missile->starsystem ) != NULL )
   {

      if( starsystem->last_missile == missile )
         starsystem->last_missile = missile->prev_in_starsystem;

      if( starsystem->first_missile == missile )
         starsystem->first_missile = missile->next_in_starsystem;

      if( missile->prev_in_starsystem )
         missile->prev_in_starsystem->next_in_starsystem = missile->next_in_starsystem;

      if( missile->next_in_starsystem )
         missile->next_in_starsystem->prev_in_starsystem = missile->prev_in_starsystem;

      missile->starsystem = NULL;
      missile->next_in_starsystem = NULL;
      missile->prev_in_starsystem = NULL;

   }

   UNLINK( missile, first_missile, last_missile, next, prev );

   missile->target = NULL;
   missile->fired_from = NULL;
   if( missile->fired_by )
      STRFREE( missile->fired_by );

   DISPOSE( missile );

}

bool is_rental( CHAR_DATA * ch, SHIP_DATA * ship )
{
   if( !str_cmp( "Public", ship->owner ) )
      return TRUE;

   return FALSE;
}

bool check_pilot( CHAR_DATA * ch, SHIP_DATA * ship )
{
   if( !str_cmp( ch->name, ship->owner ) || !str_cmp( ch->name, ship->pilot )
       || !str_cmp( ch->name, ship->copilot ) || !str_cmp( "Public", ship->owner ) )
      return TRUE;

   if( !IS_NPC( ch ) && ch->pcdata && ch->pcdata->clan )
   {
      if( !str_cmp( ch->pcdata->clan->name, ship->owner ) )
      {
         if( !str_cmp( ch->pcdata->clan->leader, ch->name ) )
            return TRUE;
         if( !str_cmp( ch->pcdata->clan->number1, ch->name ) )
            return TRUE;
         if( !str_cmp( ch->pcdata->clan->number2, ch->name ) )
            return TRUE;
         if( ch->pcdata->bestowments && is_name( "pilot", ch->pcdata->bestowments ) )
            return TRUE;
      }
   }

   return FALSE;
}

bool extract_ship( SHIP_DATA * ship )
{
   ROOM_INDEX_DATA *room;

   if( ( room = ship->in_room ) != NULL )
   {
      UNLINK( ship, room->first_ship, room->last_ship, next_in_room, prev_in_room );
      ship->in_room = NULL;
   }
   return TRUE;
}

void damage_ship_ch( SHIP_DATA * ship, int min, int max, CHAR_DATA * ch )
{
   int sdamage, shield_dmg;
   long xp;

   sdamage = number_range( min, max );

   xp = ( exp_level( ch->skill_level[PILOTING_ABILITY] + 1 ) - exp_level( ch->skill_level[PILOTING_ABILITY] ) ) / 25;
   xp = UMIN( get_ship_value( ship ) / 100, xp );
   gain_exp( ch, xp, PILOTING_ABILITY );

   if( ship->shield > 0 )
   {
      shield_dmg = UMIN( ship->shield, sdamage );
      sdamage -= shield_dmg;
      ship->shield -= shield_dmg;
      if( ship->shield == 0 )
         echo_to_cockpit( AT_BLOOD, ship, "Shields down..." );
   }

   if( sdamage > 0 )
   {
      if( number_range( 1, 100 ) <= 5 && ship->shipstate != SHIP_DISABLED )
      {
         echo_to_cockpit( AT_BLOOD + AT_BLINK, ship, "Ships Drive DAMAGED!" );
         ship->shipstate = SHIP_DISABLED;
      }

      if( number_range( 1, 100 ) <= 5 && ship->missilestate != MISSILE_DAMAGED && ship->maxmissiles > 0 )
      {
         echo_to_room( AT_BLOOD + AT_BLINK, get_room_index( ship->gunseat ), "Ships Missile Launcher DAMAGED!" );
         ship->missilestate = MISSILE_DAMAGED;
      }

      if( number_range( 1, 100 ) <= 2 && ship->statet0 != LASER_DAMAGED )
      {
         echo_to_room( AT_BLOOD + AT_BLINK, get_room_index( ship->gunseat ), "Lasers DAMAGED!" );
         ship->statet1 = LASER_DAMAGED;
      }

      if( number_range( 1, 100 ) <= 5 && ship->statet1 != LASER_DAMAGED && ship->turret1 )
      {
         echo_to_room( AT_BLOOD + AT_BLINK, get_room_index( ship->turret1 ), "Turret DAMAGED!" );
         ship->statet1 = LASER_DAMAGED;
      }

      if( number_range( 1, 100 ) <= 5 && ship->statet2 != LASER_DAMAGED && ship->turret2 )
      {
         echo_to_room( AT_BLOOD + AT_BLINK, get_room_index( ship->turret2 ), "Turret DAMAGED!" );
         ship->statet2 = LASER_DAMAGED;
      }

   }

   ship->hull -= sdamage * 5;

   if( ship->hull <= 0 )
   {
      destroy_ship( ship, ch );

      xp = ( exp_level( ch->skill_level[PILOTING_ABILITY] + 1 ) - exp_level( ch->skill_level[PILOTING_ABILITY] ) );
      xp = UMIN( get_ship_value( ship ), xp );
      gain_exp( ch, xp, PILOTING_ABILITY );
      ch_printf( ch, "&WYou gain %ld piloting experience!\r\n", xp );
      return;
   }

   if( ship->hull <= ship->maxhull / 20 )
      echo_to_cockpit( AT_BLOOD + AT_BLINK, ship, "WARNING! Ship hull severely damaged!" );

}

void damage_ship( SHIP_DATA * ship, int min, int max )
{
   int sdamage, shield_dmg;

   sdamage = number_range( min, max );

   if( ship->shield > 0 )
   {
      shield_dmg = UMIN( ship->shield, sdamage );
      sdamage -= shield_dmg;
      ship->shield -= shield_dmg;
      if( ship->shield == 0 )
         echo_to_cockpit( AT_BLOOD, ship, "Shields down..." );
   }

   if( sdamage > 0 )
   {

      if( number_range( 1, 100 ) <= 5 && ship->shipstate != SHIP_DISABLED )
      {
         echo_to_cockpit( AT_BLOOD + AT_BLINK, ship, "Ships Drive DAMAGED!" );
         ship->shipstate = SHIP_DISABLED;
      }

      if( number_range( 1, 100 ) <= 5 && ship->missilestate != MISSILE_DAMAGED && ship->maxmissiles > 0 )
      {
         echo_to_room( AT_BLOOD + AT_BLINK, get_room_index( ship->gunseat ), "Ships Missile Launcher DAMAGED!" );
         ship->missilestate = MISSILE_DAMAGED;
      }

      if( number_range( 1, 100 ) <= 2 && ship->statet1 != LASER_DAMAGED && ship->turret1 )
      {
         echo_to_room( AT_BLOOD + AT_BLINK, get_room_index( ship->turret1 ), "Turret DAMAGED!" );
         ship->statet1 = LASER_DAMAGED;
      }

      if( number_range( 1, 100 ) <= 2 && ship->statet2 != LASER_DAMAGED && ship->turret2 )
      {
         echo_to_room( AT_BLOOD + AT_BLINK, get_room_index( ship->turret2 ), "Turret DAMAGED!" );
         ship->statet2 = LASER_DAMAGED;
      }

   }

   ship->hull -= sdamage * 5;

   if( ship->hull <= 0 )
   {
      destroy_ship( ship, NULL );
      return;
   }

   if( ship->hull <= ship->maxhull / 20 )
      echo_to_cockpit( AT_BLOOD + AT_BLINK, ship, "WARNING! Ship hull severely damaged!" );

}

void destroy_ship( SHIP_DATA * ship, CHAR_DATA * ch )
{
   char buf[MAX_STRING_LENGTH];
   int roomnum;
   ROOM_INDEX_DATA *room;
   OBJ_DATA *robj;
   CHAR_DATA *rch;

   sprintf( buf, "%s explodes in a blinding flash of light!", ship->name );
   echo_to_system( AT_WHITE + AT_BLINK, ship, buf, NULL );

   if( ship->ship_class == FIGHTER_SHIP )

      echo_to_ship( AT_WHITE + AT_BLINK, ship, "A blinding flahs of light burns your eyes..." );
   echo_to_ship( AT_WHITE, ship,
                 "But before you have a schance to scream...\r\nYou are ripped apart as your spacecraft explodes..." );

   for( roomnum = ship->firstroom; roomnum <= ship->lastroom; roomnum++ )
   {
      room = get_room_index( roomnum );

      if( room != NULL )
      {
         rch = room->first_person;
         while( rch )
         {
            if( IS_IMMORTAL( rch ) )
            {
               char_from_room( rch );
               char_to_room( rch, get_room_index( wherehome( rch ) ) );
            }
            else
            {
               if( ch )
                  raw_kill( ch, rch );
               else
                  raw_kill( rch, rch );
            }
            rch = room->first_person;
         }

         for( robj = room->first_content; robj; robj = robj->next_content )
         {
            separate_obj( robj );
            extract_obj( robj );
         }
      }

   }

   resetship( ship );

}

bool ship_to_room( SHIP_DATA * ship, int vnum )
{
   ROOM_INDEX_DATA *shipto;

   if( ( shipto = get_room_index( vnum ) ) == NULL )
      return FALSE;
   LINK( ship, shipto->first_ship, shipto->last_ship, next_in_room, prev_in_room );
   ship->in_room = shipto;
   return TRUE;
}


void do_board( CHAR_DATA * ch, const char *argument )
{
   ROOM_INDEX_DATA *fromroom;
   ROOM_INDEX_DATA *toroom;
   SHIP_DATA *ship;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Board what?\r\n", ch );
      return;
   }

   if( ( ship = ship_in_room( ch->in_room, argument ) ) == NULL )
   {
      act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
      return;
   }

   if( IS_SET( ch->act, ACT_MOUNTED ) )
   {
      act( AT_PLAIN, "You can't go in there riding THAT.", ch, NULL, argument, TO_CHAR );
      return;
   }

   fromroom = ch->in_room;

   if( ( toroom = get_room_index( ship->entrance ) ) != NULL )
   {
      if( !ship->hatchopen )
      {
         send_to_char( "&RThe hatch is closed!\r\n", ch );
         return;
      }

      if( toroom->tunnel > 0 )
      {
         CHAR_DATA *ctmp;
         int count = 0;

         for( ctmp = toroom->first_person; ctmp; ctmp = ctmp->next_in_room )
            if( ++count >= toroom->tunnel )
            {
               send_to_char( "There is no room for you in there.\r\n", ch );
               return;
            }
      }
      if( ship->shipstate == SHIP_LAUNCH || ship->shipstate == SHIP_LAUNCH_2 )
      {
         send_to_char( "&rThat ship has already started launching!\r\n", ch );
         return;
      }

      act( AT_PLAIN, "$n enters $T.", ch, NULL, ship->name, TO_ROOM );
      act( AT_PLAIN, "You enter $T.", ch, NULL, ship->name, TO_CHAR );
      char_from_room( ch );
      char_to_room( ch, toroom );
      act( AT_PLAIN, "$n enters the ship.", ch, NULL, argument, TO_ROOM );
      do_look( ch, "auto" );

   }
   else
      send_to_char( "That ship has no entrance!\r\n", ch );
}

bool rent_ship( CHAR_DATA * ch, SHIP_DATA * ship )
{

   long price;

   if( IS_NPC( ch ) )
      return FALSE;

   price = get_ship_value( ship ) / 100;

   if( ch->gold < price )
   {
      ch_printf( ch, "&RRenting this ship costs %ld. You don't have enough credits!\r\n", price );
      return FALSE;
   }

   ch->gold -= price;
   ch_printf( ch, "&GYou pay %ld credits to rent the ship.\r\n", price );
   return TRUE;

}

void do_leaveship( CHAR_DATA * ch, const char *argument )
{
   ROOM_INDEX_DATA *fromroom;
   ROOM_INDEX_DATA *toroom;
   SHIP_DATA *ship;

   fromroom = ch->in_room;

   if( ( ship = ship_from_entrance( fromroom->vnum ) ) == NULL )
   {
      send_to_char( "I see no exit here.\r\n", ch );
      return;
   }

   if( ship->ship_class == SHIP_PLATFORM )
   {
      send_to_char( "You can't do that here.\r\n", ch );
      return;
   }

   if( ship->lastdoc != ship->location )
   {
      send_to_char( "&rMaybe you should wait until the ship lands.\r\n", ch );
      return;
   }

   if( ship->shipstate != SHIP_DOCKED && ship->shipstate != SHIP_DISABLED )
   {
      send_to_char( "&rPlease wait till the ship is properly docked.\r\n", ch );
      return;
   }

   if( !ship->hatchopen )
   {
      send_to_char( "&RYou need to open the hatch first", ch );
      return;
   }

   if( ( toroom = get_room_index( ship->location ) ) != NULL )
   {
      act( AT_PLAIN, "$n exits the ship.", ch, NULL, argument, TO_ROOM );
      act( AT_PLAIN, "You exit the ship.", ch, NULL, argument, TO_CHAR );
      char_from_room( ch );
      char_to_room( ch, toroom );
      act( AT_PLAIN, "$n steps out of a ship.", ch, NULL, argument, TO_ROOM );
      do_look( ch, "auto" );
   }
   else
      send_to_char( "The exit doesn't seem to be working properly.\r\n", ch );
}

void do_launch( CHAR_DATA * ch, const char *argument )
{
   int schance = 0;
   long price = 0;
   SHIP_DATA *ship;
   char buf[MAX_STRING_LENGTH];

   if( ( ship = ship_from_cockpit( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou must be in the cockpit of a ship to do that!\r\n", ch );
      return;
   }

   if( ship->ship_class > SHIP_PLATFORM )
   {
      send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
      return;
   }

   if( ( ship = ship_from_pilotseat( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou don't seem to be in the pilot seat!\r\n", ch );
      return;
   }

   if( autofly( ship ) )
   {
      send_to_char( "&RThe ship is set on autopilot, you'll have to turn it off first.\r\n", ch );
      return;
   }

   if( ship->ship_class == SHIP_PLATFORM )
   {
      send_to_char( "You can't do that here.\r\n", ch );
      return;
   }

   if( !check_pilot( ch, ship ) )
   {
      send_to_char( "&RHey, thats not your ship! Try renting a public one.\r\n", ch );
      return;
   }

   if( ship->lastdoc != ship->location )
   {
      send_to_char( "&rYou don't seem to be docked right now.\r\n", ch );
      return;
   }

   if( ship->shipstate != SHIP_DOCKED && ship->shipstate != SHIP_DISABLED )
   {
      send_to_char( "The ship is not docked right now.\r\n", ch );
      return;
   }

   if( ship->ship_class == FIGHTER_SHIP )
      schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_starfighters] );
   if( ship->ship_class == MIDSIZE_SHIP )
      schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_midships] );
   if( ship->ship_class == CAPITAL_SHIP )
      schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_capitalships] );
   if( number_percent(  ) < schance )
   {
      if( is_rental( ch, ship ) )
         if( !rent_ship( ch, ship ) )
            return;
      if( !is_rental( ch, ship ) )
      {
         if( ship->ship_class == FIGHTER_SHIP )
            price = 20;
         if( ship->ship_class == MIDSIZE_SHIP )
            price = 50;
         if( ship->ship_class == CAPITAL_SHIP )
            price = 500;

         price += ( ship->maxhull - ship->hull );
         if( ship->missiles )
            price += ( 50 * ( ship->maxmissiles - ship->missiles ) );
         else if( ship->torpedos )
            price += ( 75 * ( ship->maxtorpedos - ship->torpedos ) );
         else if( ship->rockets )
            price += ( 150 * ( ship->maxrockets - ship->rockets ) );

         if( ship->shipstate == SHIP_DISABLED )
            price += 200;
         if( ship->missilestate == MISSILE_DAMAGED )
            price += 100;
         if( ship->statet0 == LASER_DAMAGED )
            price += 50;
         if( ship->statet1 == LASER_DAMAGED )
            price += 50;
         if( ship->statet2 == LASER_DAMAGED )
            price += 50;
      }

      if( ch->pcdata && ch->pcdata->clan && !str_cmp( ch->pcdata->clan->name, ship->owner ) )
      {
         if( ch->pcdata->clan->funds < price )
         {
            ch_printf( ch, "&R%s doesn't have enough funds to prepare this ship for launch.\r\n", ch->pcdata->clan->name );
            return;
         }

         ch->pcdata->clan->funds -= price;
         ch_printf( ch, "&GIt costs %s %ld credits to ready this ship for launch.\r\n", ch->pcdata->clan->name, price );
      }
      else if( str_cmp( ship->owner, "Public" ) )
      {
         if( ch->gold < price )
         {
            ch_printf( ch, "&RYou don't have enough funds to prepare this ship for launch.\r\n" );
            return;
         }

         ch->gold -= price;
         ch_printf( ch, "&GYou pay %ld credits to ready the ship for launch.\r\n", price );

      }

      ship->energy = ship->maxenergy;
      ship->chaff = ship->maxchaff;
      ship->missiles = ship->maxmissiles;
      ship->torpedos = ship->maxtorpedos;
      ship->rockets = ship->maxrockets;
      ship->shield = 0;
      ship->autorecharge = FALSE;
      ship->autotrack = FALSE;
      ship->autospeed = FALSE;
      ship->hull = ship->maxhull;

      ship->missilestate = MISSILE_READY;
      ship->statet0 = LASER_READY;
      ship->statet1 = LASER_READY;
      ship->statet2 = LASER_READY;
      ship->shipstate = SHIP_DOCKED;

      if( ship->hatchopen )
      {
         ship->hatchopen = FALSE;
         sprintf( buf, "The hatch on %s closes.", ship->name );
         echo_to_room( AT_YELLOW, get_room_index( ship->location ), buf );
         echo_to_room( AT_YELLOW, get_room_index( ship->entrance ), "The hatch slides shut." );
         sound_to_room( get_room_index( ship->entrance ), "!!SOUND(door)" );
         sound_to_room( get_room_index( ship->location ), "!!SOUND(door)" );
      }
      set_char_color( AT_GREEN, ch );
      send_to_char( "Launch sequence initiated.\r\n", ch );
      act( AT_PLAIN, "$n starts up the ship and begins the launch sequence.", ch, NULL, argument, TO_ROOM );
      echo_to_ship( AT_YELLOW, ship, "The ship hums as it lifts off the ground." );
      sprintf( buf, "%s begins to launch.", ship->name );
      echo_to_room( AT_YELLOW, get_room_index( ship->location ), buf );
      ship->shipstate = SHIP_LAUNCH;
      ship->currspeed = ship->realspeed;
      if( ship->ship_class == FIGHTER_SHIP )
         learn_from_success( ch, gsn_starfighters );
      if( ship->ship_class == MIDSIZE_SHIP )
         learn_from_success( ch, gsn_midships );
      if( ship->ship_class == CAPITAL_SHIP )
         learn_from_success( ch, gsn_capitalships );
      sound_to_ship( ship, "!!SOUND(xwing)" );
      return;
   }
   set_char_color( AT_RED, ch );
   send_to_char( "You fail to work the controls properly!\r\n", ch );
   if( ship->ship_class == FIGHTER_SHIP )
      learn_from_failure( ch, gsn_starfighters );
   if( ship->ship_class == MIDSIZE_SHIP )
      learn_from_failure( ch, gsn_midships );
   if( ship->ship_class == CAPITAL_SHIP )
      learn_from_failure( ch, gsn_capitalships );
   return;

}

void launchship( SHIP_DATA * ship )
{
   char buf[MAX_STRING_LENGTH];
   SHIP_DATA *target;
   int plusminus;

   ship_to_starsystem( ship, starsystem_from_vnum( ship->location ) );


   if( ship->starsystem == NULL )
   {
      echo_to_room( AT_YELLOW, get_room_index( ship->pilotseat ), "Launch path blocked .. Launch aborted." );
      echo_to_ship( AT_YELLOW, ship, "The ship slowly sets back back down on the landing pad." );
      sprintf( buf, "%s slowly sets back down.", ship->name );
      echo_to_room( AT_YELLOW, get_room_index( ship->location ), buf );
      ship->shipstate = SHIP_DOCKED;
      return;
   }

   if( ship->ship_class == MIDSIZE_SHIP )
   {
      sound_to_room( get_room_index( ship->location ), "!!SOUND(falcon)" );
      sound_to_ship( ship, "!!SOUND(falcon)" );
   }
   else if( ship->type == SHIP_IMPERIAL )
   {
      sound_to_ship( ship, "!!SOUND(tie)" );
      sound_to_room( get_room_index( ship->location ), "!!SOUND(tie)" );
   }
   else
   {
      sound_to_ship( ship, "!!SOUND(xwing)" );
      sound_to_room( get_room_index( ship->location ), "!!SOUND(xwing)" );
   }

   extract_ship( ship );

   ship->location = 0;

   if( ship->shipstate != SHIP_DISABLED )
      ship->shipstate = SHIP_READY;

   plusminus = number_range( -1, 2 );
   if( plusminus > 0 )
      ship->hx = 1;
   else
      ship->hx = -1;

   plusminus = number_range( -1, 2 );
   if( plusminus > 0 )
      ship->hy = 1;
   else
      ship->hy = -1;

   plusminus = number_range( -1, 2 );
   if( plusminus > 0 )
      ship->hz = 1;
   else
      ship->hz = -1;

   if( ship->lastdoc == ship->starsystem->doc1a ||
       ship->lastdoc == ship->starsystem->doc1b || ship->lastdoc == ship->starsystem->doc1c )
   {
      ship->vx = ship->starsystem->p1x;
      ship->vy = ship->starsystem->p1y;
      ship->vz = ship->starsystem->p1z;
   }
   else if( ship->lastdoc == ship->starsystem->doc2a ||
            ship->lastdoc == ship->starsystem->doc2b || ship->lastdoc == ship->starsystem->doc2c )
   {
      ship->vx = ship->starsystem->p2x;
      ship->vy = ship->starsystem->p2y;
      ship->vz = ship->starsystem->p2z;
   }
   else if( ship->lastdoc == ship->starsystem->doc3a ||
            ship->lastdoc == ship->starsystem->doc3b || ship->lastdoc == ship->starsystem->doc3c )
   {
      ship->vx = ship->starsystem->p3x;
      ship->vy = ship->starsystem->p3y;
      ship->vz = ship->starsystem->p3z;
   }
   else
   {
      for( target = ship->starsystem->first_ship; target; target = target->next_in_starsystem )
      {
         if( ship->lastdoc == target->hanger )
         {
            ship->vx = target->vx;
            ship->vy = target->vy;
            ship->vz = target->vz;
         }
      }
   }

   ship->energy -= ( 100 + 100 * ship->ship_class );

   ship->vx += ( ship->hx * ship->currspeed * 2 );
   ship->vy += ( ship->hy * ship->currspeed * 2 );
   ship->vz += ( ship->hz * ship->currspeed * 2 );

   echo_to_room( AT_GREEN, get_room_index( ship->pilotseat ), "Launch complete.\r\n" );
   echo_to_ship( AT_YELLOW, ship, "The ship leaves the platform far behind as it flies into space." );
   sprintf( buf, "%s enters the starsystem at %.0f %.0f %.0f", ship->name, ship->vx, ship->vy, ship->vz );
   echo_to_system( AT_YELLOW, ship, buf, NULL );
   sprintf( buf, "%s lifts off into space.", ship->name );
   echo_to_room( AT_YELLOW, get_room_index( ship->lastdoc ), buf );

}


void do_land( CHAR_DATA * ch, const char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   int schance = 0;
   SHIP_DATA *ship;
   SHIP_DATA *target;
   int vx = 0, vy = 0, vz = 0;

   strcpy( arg, argument );

   if( ( ship = ship_from_cockpit( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou must be in the cockpit of a ship to do that!\r\n", ch );
      return;
   }

   if( ship->ship_class > SHIP_PLATFORM )
   {
      send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
      return;
   }

   if( ( ship = ship_from_pilotseat( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou need to be in the pilot seat!\r\n", ch );
      return;
   }

   if( autofly( ship ) )
   {
      send_to_char( "&RYou'll have to turn off the ships autopilot first.\r\n", ch );
      return;
   }

   if( ship->ship_class == SHIP_PLATFORM )
   {
      send_to_char( "&RYou can't land platforms\r\n", ch );
      return;
   }

   if( ship->ship_class == CAPITAL_SHIP )
   {
      send_to_char( "&RCapital ships are to big to land. You'll have to take a shuttle.\r\n", ch );
      return;
   }
   if( ship->shipstate == SHIP_DISABLED )
   {
      send_to_char( "&RThe ships drive is disabled. Unable to land.\r\n", ch );
      return;
   }
   if( ship->shipstate == SHIP_DOCKED )
   {
      send_to_char( "&RThe ship is already docked!\r\n", ch );
      return;
   }

   if( ship->shipstate == SHIP_HYPERSPACE )
   {
      send_to_char( "&RYou can only do that in realspace!\r\n", ch );
      return;
   }

   if( ship->shipstate != SHIP_READY )
   {
      send_to_char( "&RPlease wait until the ship has finished its current manouver.\r\n", ch );
      return;
   }
   if( ship->starsystem == NULL )
   {
      send_to_char( "&RThere's nowhere to land around here!", ch );
      return;
   }

   if( ship->energy < ( 25 + 25 * ship->ship_class ) )
   {
      send_to_char( "&RTheres not enough fuel!\r\n", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      set_char_color( AT_CYAN, ch );
      ch_printf( ch, "%s", "Land where?\r\n\r\nChoices: " );

      if( ship->starsystem->doc1a )
         ch_printf( ch, "%s (%s)  %d %d %d\r\n         ",
                    ship->starsystem->location1a,
                    ship->starsystem->planet1, ship->starsystem->p1x, ship->starsystem->p1y, ship->starsystem->p1z );
      if( ship->starsystem->doc1b )
         ch_printf( ch, "%s (%s)  %d %d %d\r\n         ",
                    ship->starsystem->location1b,
                    ship->starsystem->planet1, ship->starsystem->p1x, ship->starsystem->p1y, ship->starsystem->p1z );
      if( ship->starsystem->doc1c )
         ch_printf( ch, "%s (%s)  %d %d %d\r\n         ",
                    ship->starsystem->location1c,
                    ship->starsystem->planet1, ship->starsystem->p1x, ship->starsystem->p1y, ship->starsystem->p1z );
      if( ship->starsystem->doc2a )
         ch_printf( ch, "%s (%s)  %d %d %d\r\n         ",
                    ship->starsystem->location2a,
                    ship->starsystem->planet2, ship->starsystem->p2x, ship->starsystem->p2y, ship->starsystem->p2z );
      if( ship->starsystem->doc2b )
         ch_printf( ch, "%s (%s)  %d %d %d\r\n         ",
                    ship->starsystem->location2b,
                    ship->starsystem->planet2, ship->starsystem->p2x, ship->starsystem->p2y, ship->starsystem->p2z );
      if( ship->starsystem->doc2c )
         ch_printf( ch, "%s (%s)  %d %d %d\r\n         ",
                    ship->starsystem->location2c,
                    ship->starsystem->planet2, ship->starsystem->p2x, ship->starsystem->p2y, ship->starsystem->p2z );
      if( ship->starsystem->doc3a )
         ch_printf( ch, "%s (%s)  %d %d %d\r\n         ",
                    ship->starsystem->location3a,
                    ship->starsystem->planet3, ship->starsystem->p3x, ship->starsystem->p3y, ship->starsystem->p3z );
      if( ship->starsystem->doc3b )
         ch_printf( ch, "%s (%s)  %d %d %d\r\n         ",
                    ship->starsystem->location3b,
                    ship->starsystem->planet3, ship->starsystem->p3x, ship->starsystem->p3y, ship->starsystem->p3z );
      if( ship->starsystem->doc3c )
         ch_printf( ch, "%s (%s)  %d %d %d\r\n         ",
                    ship->starsystem->location3c,
                    ship->starsystem->planet3, ship->starsystem->p3x, ship->starsystem->p3y, ship->starsystem->p3z );
      for( target = ship->starsystem->first_ship; target; target = target->next_in_starsystem )
      {
         if( target->hanger > 0 && target != ship )
            ch_printf( ch, "%s    %.0f %.0f %.0f\r\n         ", target->name, target->vx, target->vy, target->vz );
      }
      ch_printf( ch, "\r\nYour Coordinates: %.0f %.0f %.0f\r\n", ship->vx, ship->vy, ship->vz );
      return;
   }

   if( str_prefix( argument, ship->starsystem->location1a ) &&
       str_prefix( argument, ship->starsystem->location2a ) &&
       str_prefix( argument, ship->starsystem->location3a ) &&
       str_prefix( argument, ship->starsystem->location1b ) &&
       str_prefix( argument, ship->starsystem->location2b ) &&
       str_prefix( argument, ship->starsystem->location3b ) &&
       str_prefix( argument, ship->starsystem->location1c ) &&
       str_prefix( argument, ship->starsystem->location2c ) && str_prefix( argument, ship->starsystem->location3c ) )
   {
      target = get_ship_here( argument, ship->starsystem );
      if( target == NULL )
      {
         send_to_char( "&RI don't see that here. Type land by itself for a list\r\n", ch );
         return;
      }
      if( target == ship )
      {
         send_to_char( "&RYou can't land your ship inside itself!\r\n", ch );
         return;
      }
      if( !target->hanger )
      {
         send_to_char( "&RThat ship has no hanger for you to land in!\r\n", ch );
         return;
      }
      if( ship->ship_class == MIDSIZE_SHIP && target->ship_class == MIDSIZE_SHIP )
      {
         send_to_char( "&RThat ship is not big enough for your ship to land in!\r\n", ch );
         return;
      }
      if( !target->bayopen )
      {
         send_to_char( "&RTheir hanger is closed. You'll have to ask them to open it for you\r\n", ch );
         return;
      }
      if( ( target->vx > ship->vx + 200 ) || ( target->vx < ship->vx - 200 ) ||
          ( target->vy > ship->vy + 200 ) || ( target->vy < ship->vy - 200 ) ||
          ( target->vz > ship->vz + 200 ) || ( target->vz < ship->vz - 200 ) )
      {
         send_to_char( "&R That ship is too far away! You'll have to fly a litlle closer.\r\n", ch );
         return;
      }
   }
   else
   {
      if( !str_prefix( argument, ship->starsystem->location3a ) ||
          !str_prefix( argument, ship->starsystem->location3b ) || !str_prefix( argument, ship->starsystem->location3c ) )
      {
         vx = ship->starsystem->p3x;
         vy = ship->starsystem->p3y;
         vz = ship->starsystem->p3z;
      }
      if( !str_prefix( argument, ship->starsystem->location2a ) ||
          !str_prefix( argument, ship->starsystem->location2b ) || !str_prefix( argument, ship->starsystem->location2c ) )
      {
         vx = ship->starsystem->p2x;
         vy = ship->starsystem->p2y;
         vz = ship->starsystem->p2z;
      }
      if( !str_prefix( argument, ship->starsystem->location1a ) ||
          !str_prefix( argument, ship->starsystem->location1b ) || !str_prefix( argument, ship->starsystem->location1c ) )
      {
         vx = ship->starsystem->p1x;
         vy = ship->starsystem->p1y;
         vz = ship->starsystem->p1z;
      }
      if( ( vx > ship->vx + 200 ) || ( vx < ship->vx - 200 ) ||
          ( vy > ship->vy + 200 ) || ( vy < ship->vy - 200 ) || ( vz > ship->vz + 200 ) || ( vz < ship->vz - 200 ) )
      {
         send_to_char( "&R That platform is too far away! You'll have to fly a litlle closer.\r\n", ch );
         return;
      }
   }

   if( ship->ship_class == FIGHTER_SHIP )
      schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_starfighters] );
   if( ship->ship_class == MIDSIZE_SHIP )
      schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_midships] );
   if( number_percent(  ) < schance )
   {
      set_char_color( AT_GREEN, ch );
      send_to_char( "Landing sequence initiated.\r\n", ch );
      act( AT_PLAIN, "$n begins the landing sequence.", ch, NULL, argument, TO_ROOM );
      echo_to_ship( AT_YELLOW, ship, "The ship slowly begins its landing aproach." );
      ship->dest = STRALLOC( arg );
      ship->shipstate = SHIP_LAND;
      ship->currspeed = 0;
      if( ship->ship_class == FIGHTER_SHIP )
         learn_from_success( ch, gsn_starfighters );
      if( ship->ship_class == MIDSIZE_SHIP )
         learn_from_success( ch, gsn_midships );
      if( starsystem_from_vnum( ship->lastdoc ) != ship->starsystem )
      {
         int xp = ( exp_level( ch->skill_level[PILOTING_ABILITY] + 1 ) - exp_level( ch->skill_level[PILOTING_ABILITY] ) );
         xp = UMIN( get_ship_value( ship ), xp );
         gain_exp( ch, xp, PILOTING_ABILITY );
         ch_printf( ch, "&WYou gain %ld points of flight experience!\r\n", UMIN( get_ship_value( ship ), xp ) );
      }
      return;
   }
   send_to_char( "You fail to work the controls properly.\r\n", ch );
   if( ship->ship_class == FIGHTER_SHIP )
      learn_from_failure( ch, gsn_starfighters );
   else
      learn_from_failure( ch, gsn_midships );
   return;
}

void landship( SHIP_DATA * ship, const char *arg )
{
   SHIP_DATA *target;
   char buf[MAX_STRING_LENGTH];
   int destination = 0;

   if( !str_prefix( arg, ship->starsystem->location3a ) )
      destination = ship->starsystem->doc3a;
   if( !str_prefix( arg, ship->starsystem->location3b ) )
      destination = ship->starsystem->doc3b;
   if( !str_prefix( arg, ship->starsystem->location3c ) )
      destination = ship->starsystem->doc3c;
   if( !str_prefix( arg, ship->starsystem->location2a ) )
      destination = ship->starsystem->doc2a;
   if( !str_prefix( arg, ship->starsystem->location2b ) )
      destination = ship->starsystem->doc2b;
   if( !str_prefix( arg, ship->starsystem->location2c ) )
      destination = ship->starsystem->doc2c;
   if( !str_prefix( arg, ship->starsystem->location1a ) )
      destination = ship->starsystem->doc1a;
   if( !str_prefix( arg, ship->starsystem->location1b ) )
      destination = ship->starsystem->doc1b;
   if( !str_prefix( arg, ship->starsystem->location1c ) )
      destination = ship->starsystem->doc1c;

   target = get_ship_here( arg, ship->starsystem );
   if( target != ship && target != NULL && target->bayopen
       && ( ship->ship_class != MIDSIZE_SHIP || target->ship_class != MIDSIZE_SHIP ) )
      destination = target->hanger;

   if( !ship_to_room( ship, destination ) )
   {
      echo_to_room( AT_YELLOW, get_room_index( ship->pilotseat ), "Could not complete aproach. Landing aborted." );
      echo_to_ship( AT_YELLOW, ship, "The ship pulls back up out of its landing sequence." );
      if( ship->shipstate != SHIP_DISABLED )
         ship->shipstate = SHIP_READY;
      return;
   }

   echo_to_room( AT_YELLOW, get_room_index( ship->pilotseat ), "Landing sequence complete." );
   echo_to_ship( AT_YELLOW, ship, "You feel a slight thud as the ship sets down on the ground." );
   sprintf( buf, "%s disapears from your scanner.", ship->name );
   echo_to_system( AT_YELLOW, ship, buf, NULL );

   ship->location = destination;
   ship->lastdoc = ship->location;
   if( ship->shipstate != SHIP_DISABLED )
      ship->shipstate = SHIP_DOCKED;
   ship_from_starsystem( ship, ship->starsystem );

   sprintf( buf, "%s lands on the platform.", ship->name );
   echo_to_room( AT_YELLOW, get_room_index( ship->location ), buf );

   ship->energy = ship->energy - 25 - 25 * ship->ship_class;

   if( !str_cmp( "Public", ship->owner ) )
   {
      ship->energy = ship->maxenergy;
      ship->chaff = ship->maxchaff;
      ship->missiles = ship->maxmissiles;
      ship->torpedos = ship->maxtorpedos;
      ship->rockets = ship->maxrockets;
      ship->shield = 0;
      ship->autorecharge = FALSE;
      ship->autotrack = FALSE;
      ship->autospeed = FALSE;
      ship->hull = ship->maxhull;

      ship->missilestate = MISSILE_READY;
      ship->statet0 = LASER_READY;
      ship->statet1 = LASER_READY;
      ship->statet2 = LASER_READY;
      ship->shipstate = SHIP_DOCKED;

      echo_to_cockpit( AT_YELLOW, ship, "Repairing and refueling ship..." );
   }

   save_ship( ship );
}

void do_accelerate( CHAR_DATA * ch, const char *argument )
{
   int schance = 0;
   int change;
   SHIP_DATA *ship;
   char buf[MAX_STRING_LENGTH];

   if( ( ship = ship_from_cockpit( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou must be in the cockpit of a ship to do that!\r\n", ch );
      return;
   }

   if( ship->ship_class > SHIP_PLATFORM )
   {
      send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
      return;
   }

   if( ( ship = ship_from_pilotseat( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RThe controls must be at the pilots chair...\r\n", ch );
      return;
   }

   if( autofly( ship ) )
   {
      send_to_char( "&RYou'll have to turn off the ships autopilot first.\r\n", ch );
      return;
   }

   if( ship->ship_class == SHIP_PLATFORM )
   {
      send_to_char( "&RPlatforms can't move!\r\n", ch );
      return;
   }

   if( ship->shipstate == SHIP_HYPERSPACE )
   {
      send_to_char( "&RYou can only do that in realspace!\r\n", ch );
      return;
   }
   if( ship->shipstate == SHIP_DISABLED )
   {
      send_to_char( "&RThe ships drive is disabled. Unable to accelerate.\r\n", ch );
      return;
   }
   if( ship->shipstate == SHIP_DOCKED )
   {
      send_to_char( "&RYou can't do that until after you've launched!\r\n", ch );
      return;
   }
   if( ship->energy < abs( ( atoi( argument ) - abs( ship->currspeed ) ) / 10 ) )
   {
      send_to_char( "&RTheres not enough fuel!\r\n", ch );
      return;
   }

   if( ship->ship_class == FIGHTER_SHIP )
      schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_starfighters] );
   if( ship->ship_class == MIDSIZE_SHIP )
      schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_midships] );
   if( ship->ship_class == CAPITAL_SHIP )
      schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_capitalships] );
   if( number_percent(  ) >= schance )
   {
      send_to_char( "&RYou fail to work the controls properly.\r\n", ch );
      if( ship->ship_class == FIGHTER_SHIP )
         learn_from_failure( ch, gsn_starfighters );
      if( ship->ship_class == MIDSIZE_SHIP )
         learn_from_failure( ch, gsn_midships );
      if( ship->ship_class == CAPITAL_SHIP )
         learn_from_failure( ch, gsn_capitalships );
      return;
   }

   change = atoi( argument );

   act( AT_PLAIN, "$n manipulates the ships controls.", ch, NULL, argument, TO_ROOM );

   if( change > ship->currspeed )
   {
      send_to_char( "&GAccelerating\r\n", ch );
      echo_to_cockpit( AT_YELLOW, ship, "The ship begins to accelerate." );
      sprintf( buf, "%s begins to speed up.", ship->name );
      echo_to_system( AT_ORANGE, ship, buf, NULL );
   }

   if( change < ship->currspeed )
   {
      send_to_char( "&GDecelerating\r\n", ch );
      echo_to_cockpit( AT_YELLOW, ship, "The ship begins to slow down." );
      sprintf( buf, "%s begins to slow down.", ship->name );
      echo_to_system( AT_ORANGE, ship, buf, NULL );
   }

   ship->energy -= abs( ( change - abs( ship->currspeed ) ) / 10 );

   ship->currspeed = URANGE( 0, change, ship->realspeed );

   if( ship->ship_class == FIGHTER_SHIP )
      learn_from_success( ch, gsn_starfighters );
   if( ship->ship_class == MIDSIZE_SHIP )
      learn_from_success( ch, gsn_midships );
   if( ship->ship_class == CAPITAL_SHIP )
      learn_from_success( ch, gsn_capitalships );

}

void do_trajectory( CHAR_DATA * ch, const char *argument )
{
   char buf[MAX_STRING_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   char arg3[MAX_INPUT_LENGTH];
   int schance = 0;
   float vx, vy, vz;
   SHIP_DATA *ship;

   if( ( ship = ship_from_cockpit( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou must be in the cockpit of a ship to do that!\r\n", ch );
      return;
   }

   if( ship->ship_class > SHIP_PLATFORM )
   {
      send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
      return;
   }

   if( ( ship = ship_from_pilotseat( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYour not in the pilots seat.\r\n", ch );
      return;
   }

   if( autofly( ship ) )
   {
      send_to_char( "&RYou'll have to turn off the ships autopilot first.\r\n", ch );
      return;
   }

   if( ship->shipstate == SHIP_DISABLED )
   {
      send_to_char( "&RThe ships drive is disabled. Unable to manuever.\r\n", ch );
      return;
   }

   if( ship->ship_class == SHIP_PLATFORM )
   {
      send_to_char( "&RPlatforms can't turn!\r\n", ch );
      return;
   }

   if( ship->shipstate == SHIP_HYPERSPACE )
   {
      send_to_char( "&RYou can only do that in realspace!\r\n", ch );
      return;
   }

   if( ship->shipstate == SHIP_DOCKED )
   {
      send_to_char( "&RYou can't do that until after you've launched!\r\n", ch );
      return;
   }

   if( ship->shipstate != SHIP_READY )
   {
      send_to_char( "&RPlease wait until the ship has finished its current manouver.\r\n", ch );
      return;
   }

   if( ship->energy < ( ship->currspeed / 10 ) )
   {
      send_to_char( "&RTheres not enough fuel!\r\n", ch );
      return;
   }

   if( ship->ship_class == FIGHTER_SHIP )
      schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_starfighters] );
   if( ship->ship_class == MIDSIZE_SHIP )
      schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_midships] );
   if( ship->ship_class == CAPITAL_SHIP )
      schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_capitalships] );
   if( number_percent(  ) > schance )
   {
      send_to_char( "&RYou fail to work the controls properly.\r\n", ch );
      if( ship->ship_class == FIGHTER_SHIP )
         learn_from_failure( ch, gsn_starfighters );
      if( ship->ship_class == MIDSIZE_SHIP )
         learn_from_failure( ch, gsn_midships );
      if( ship->ship_class == CAPITAL_SHIP )
         learn_from_failure( ch, gsn_capitalships );
      return;
   }

   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );

   vx = atof( arg2 );
   vy = atof( arg3 );
   vz = atof( argument );

   if( vx == ship->vx && vy == ship->vy && vz == ship->vz )
   {
      ch_printf( ch, "The ship is already at %.0f %.0f %.0f !", vx, vy, vz );
   }

   ship->hx = vx - ship->vx;
   ship->hy = vy - ship->vy;
   ship->hz = vz - ship->vz;

   ship->energy -= ( ship->currspeed / 10 );

   ch_printf( ch, "&GNew course set, aproaching %.0f %.0f %.0f.\r\n", vx, vy, vz );
   act( AT_PLAIN, "$n manipulates the ships controls.", ch, NULL, argument, TO_ROOM );

   echo_to_cockpit( AT_YELLOW, ship, "The ship begins to turn.\r\n" );
   sprintf( buf, "%s turns altering its present course.", ship->name );
   echo_to_system( AT_ORANGE, ship, buf, NULL );

   if( ship->ship_class == FIGHTER_SHIP || ( ship->ship_class == MIDSIZE_SHIP && ship->manuever > 50 ) )
      ship->shipstate = SHIP_BUSY_3;
   else if( ship->ship_class == MIDSIZE_SHIP || ( ship->ship_class == CAPITAL_SHIP && ship->manuever > 50 ) )
      ship->shipstate = SHIP_BUSY_2;
   else
      ship->shipstate = SHIP_BUSY;

   if( ship->ship_class == FIGHTER_SHIP )
      learn_from_success( ch, gsn_starfighters );
   if( ship->ship_class == MIDSIZE_SHIP )
      learn_from_success( ch, gsn_midships );
   if( ship->ship_class == CAPITAL_SHIP )
      learn_from_success( ch, gsn_capitalships );
}

void do_buyship( CHAR_DATA * ch, const char *argument )
{
   long price;
   SHIP_DATA *ship;

   if( IS_NPC( ch ) || !ch->pcdata )
   {
      send_to_char( "&ROnly players can do that!\r\n", ch );
      return;
   }

   ship = ship_in_room( ch->in_room, argument );
   if( !ship )
   {
      ship = ship_from_cockpit( ch->in_room->vnum );

      if( !ship )
      {
         act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
         return;
      }
   }

   if( str_cmp( ship->owner, "" ) || ship->type == MOB_SHIP )
   {
      send_to_char( "&RThat ship isn't for sale!", ch );
      return;
   }


   if( ship->type == SHIP_IMPERIAL )
   {
      if( !ch->pcdata->clan || str_cmp( ch->pcdata->clan->name, "the empire" ) )
      {
         if( !ch->pcdata->clan || !ch->pcdata->clan->mainclan || str_cmp( ch->pcdata->clan->mainclan->name, "The Empire" ) )
         {
            send_to_char( "&RThat ship may only be purchaced by the Empire!\r\n", ch );
            return;
         }
      }
   }
   else if( ship->type == SHIP_REPUBLIC )
   {
      if( !ch->pcdata->clan || str_cmp( ch->pcdata->clan->name, "the new republic" ) )
      {
         if( !ch->pcdata->clan || !ch->pcdata->clan->mainclan
             || str_cmp( ch->pcdata->clan->mainclan->name, "The New Republic" ) )
         {
            send_to_char( "&RThat ship may only be purchaced by The New Republic!\r\n", ch );
            return;
         }
      }
   }
   else
   {
      if( ch->pcdata->clan &&
          ( !str_cmp( ch->pcdata->clan->name, "the new republic" ) ||
            ( ch->pcdata->clan->mainclan && !str_cmp( ch->pcdata->clan->mainclan->name, "the new republic" ) ) ) )
      {
         send_to_char( "&RAs a member of the New Republic you may only purchase NR Ships!\r\n", ch );
         return;
      }
      if( ch->pcdata->clan &&
          ( !str_cmp( ch->pcdata->clan->name, "the empire" ) ||
            ( ch->pcdata->clan->mainclan && !str_cmp( ch->pcdata->clan->mainclan->name, "the empire" ) ) ) )
      {
         send_to_char( "&RAs a member of the Empire you may only purchase Imperial Ships!\r\n", ch );
         return;
      }
   }

   price = get_ship_value( ship );

   if( ch->gold < price )
   {
      ch_printf( ch, "&RThis ship costs %ld. You don't have enough credits!\r\n", price );
      return;
   }

   ch->gold -= price;
   ch_printf( ch, "&GYou pay %ld credits to purchace the ship.\r\n", price );

   act( AT_PLAIN, "$n walks over to a terminal and makes a credit transaction.", ch, NULL, argument, TO_ROOM );

   STRFREE( ship->owner );
   ship->owner = STRALLOC( ch->name );
   save_ship( ship );

}

void do_clanbuyship( CHAR_DATA * ch, const char *argument )
{
   long price;
   SHIP_DATA *ship;
   CLAN_DATA *clan;
   CLAN_DATA *mainclan;

   if( IS_NPC( ch ) || !ch->pcdata )
   {
      send_to_char( "&ROnly players can do that!\r\n", ch );
      return;
   }
   if( !ch->pcdata->clan )
   {
      send_to_char( "&RYou aren't a member of any organizations!\r\n", ch );
      return;
   }

   clan = ch->pcdata->clan;
   mainclan = ch->pcdata->clan->mainclan ? ch->pcdata->clan->mainclan : clan;

   if( ( ch->pcdata->bestowments
         && is_name( "clanbuyship", ch->pcdata->bestowments ) ) || !str_cmp( ch->name, clan->leader ) )
      ;
   else
   {
      send_to_char( "&RYour organization hasn't seen fit to bestow you with that ability.\r\n", ch );
      return;
   }

   ship = ship_in_room( ch->in_room, argument );
   if( !ship )
   {
      ship = ship_from_cockpit( ch->in_room->vnum );

      if( !ship )
      {
         act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
         return;
      }
   }

   if( str_cmp( ship->owner, "" ) || ship->type == MOB_SHIP )
   {
      send_to_char( "&RThat ship isn't for sale!\r\n", ch );
      return;
   }

   if( str_cmp( mainclan->name, "The Empire" ) && ship->type == SHIP_IMPERIAL )
   {
      send_to_char( "&RThat ship may only be purchaced by the Empire!\r\n", ch );
      return;
   }

   if( str_cmp( mainclan->name, "The New Republic" ) && ship->type == SHIP_REPUBLIC )
   {
      send_to_char( "&RThat ship may only be purchaced by The New Republic!\r\n", ch );
      return;
   }

   if( !str_cmp( mainclan->name, "The Empire" ) && ship->type != SHIP_IMPERIAL )
   {
      send_to_char( "&RDue to contractual agreements that ship may not be purchaced by the empire!\r\n", ch );
      return;
   }

   if( !str_cmp( mainclan->name, "The New Republic" ) && ship->type != SHIP_REPUBLIC )
   {
      send_to_char( "&RBecause of contractual agreements, the NR can only purchase NR ships!\r\n", ch );
      return;
   }

   price = get_ship_value( ship );

   if( ch->pcdata->clan->funds < price )
   {
      ch_printf( ch, "&RThis ship costs %ld. You don't have enough credits!\r\n", price );
      return;
   }

   clan->funds -= price;
   ch_printf( ch, "&G%s pays %ld credits to purchace the ship.\r\n", clan->name, price );

   act( AT_PLAIN, "$n walks over to a terminal and makes a credit transaction.", ch, NULL, argument, TO_ROOM );

   STRFREE( ship->owner );
   ship->owner = STRALLOC( clan->name );
   save_ship( ship );

   if( ship->ship_class <= SHIP_PLATFORM )
      clan->spacecraft++;
   else
      clan->vehicles++;
}

void do_sellship( CHAR_DATA * ch, const char *argument )
{
   long price;
   SHIP_DATA *ship;

   ship = ship_in_room( ch->in_room, argument );
   if( !ship )
   {
      act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
      return;
   }

   if( str_cmp( ship->owner, ch->name ) )
   {
      send_to_char( "&RThat isn't your ship!", ch );
      return;
   }

   price = get_ship_value( ship );

   ch->gold += ( price - price / 10 );
   ch_printf( ch, "&GYou receive %ld credits from selling your ship.\r\n", price - price / 10 );

   act( AT_PLAIN, "$n walks over to a terminal and makes a credit transaction.", ch, NULL, argument, TO_ROOM );

   STRFREE( ship->owner );
   ship->owner = STRALLOC( "" );
   save_ship( ship );

}

void do_info( CHAR_DATA * ch, const char *argument )
{
   SHIP_DATA *ship;
   SHIP_DATA *target;

   if( ( ship = ship_from_cockpit( ch->in_room->vnum ) ) == NULL )
   {
      if( argument[0] == '\0' )
      {
         act( AT_PLAIN, "Which ship do you want info on?.", ch, NULL, NULL, TO_CHAR );
         return;
      }

      ship = ship_in_room( ch->in_room, argument );
      if( !ship )
      {
         act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
         return;
      }

      target = ship;
   }
   else if( argument[0] == '\0' )
      target = ship;
   else
      target = get_ship_here( argument, ship->starsystem );

   if( target == NULL )
   {
      send_to_char( "&RI don't see that here.\r\nTry the radar, or type info by itself for info on this ship.\r\n", ch );
      return;
   }

   if( abs( ( int )( target->vx - ship->vx ) ) > 500 + ship->sensor * 2
       || abs( ( int )( target->vy - ship->vy ) ) > 500 + ship->sensor * 2
       || abs( ( int )( target->vz - ship->vz ) ) > 500 + ship->sensor * 2 )
   {
      send_to_char( "&RThat ship is to far away to scan.\r\n", ch );
      return;
   }

   ch_printf( ch, "&Y%s %s : %s\r\n&B",
              target->type == SHIP_REPUBLIC ? "New Republic" :
              ( target->type == SHIP_IMPERIAL ? "Imperial" : "Civilian" ),
              target->ship_class == FIGHTER_SHIP ? "Starfighter" :
              ( target->ship_class == MIDSIZE_SHIP ? "Midtarget" :
                ( target->ship_class == CAPITAL_SHIP ? "Capital Ship" :
                  ( ship->ship_class == SHIP_PLATFORM ? "Platform" :
                    ( ship->ship_class == CLOUD_CAR ? "Cloudcar" :
                      ( ship->ship_class == OCEAN_SHIP ? "Boat" :
                        ( ship->ship_class == LAND_SPEEDER ? "Speeder" :
                          ( ship->ship_class == WHEELED ? "Wheeled Transport" :
                            ( ship->ship_class == LAND_CRAWLER ? "Crawler" :
                              ( ship->ship_class == WALKER ? "Walker" : "Unknown" ) ) ) ) ) ) ) ) ),
              target->name, target->filename );
   ch_printf( ch, "Description: %s\r\nOwner: %s   Pilot: %s   Copilot: %s\r\n",
              target->description, target->owner, target->pilot, target->copilot );
   ch_printf( ch, "Laser cannons: %d  ", target->lasers );
   ch_printf( ch, "Maximum Missiles: %d  ", target->maxmissiles );
   ch_printf( ch, "Max Chaff: %d\r\n", target->maxchaff );
   ch_printf( ch, "Max Hull: %d  ", target->maxhull );
   ch_printf( ch, "Max Shields: %d   Max Energy(fuel): %d\r\n", target->maxshield, target->maxenergy );
   ch_printf( ch, "Maximum Speed: %d   Hyperspeed: %d\r\n", target->realspeed, target->hyperspeed );

   act( AT_PLAIN, "$n checks various gages and displays on the control panel.", ch, NULL, argument, TO_ROOM );

}

void do_autorecharge( CHAR_DATA * ch, const char *argument )
{
   int schance;
   SHIP_DATA *ship;
   int recharge;


   if( ( ship = ship_from_cockpit( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou must be in the cockpit of a ship to do that!\r\n", ch );
      return;
   }

   if( ( ship = ship_from_coseat( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou must be in the co-pilots seat!\r\n", ch );
      return;
   }

   if( autofly( ship ) )
   {
      send_to_char( "&RYou'll have to turn off the ships autopilot first.\r\n", ch );
      return;
   }

   schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_shipsystems] );
   if( number_percent(  ) > schance )
   {
      send_to_char( "&RYou fail to work the controls properly.\r\n", ch );
      learn_from_failure( ch, gsn_shipsystems );
      return;
   }

   act( AT_PLAIN, "$n flips a switch on the control panell.", ch, NULL, argument, TO_ROOM );

   if( !str_cmp( argument, "on" ) )
   {
      ship->autorecharge = TRUE;
      send_to_char( "&GYou power up the shields.\r\n", ch );
      echo_to_cockpit( AT_YELLOW, ship, "Shields ON. Autorecharge ON." );
   }
   else if( !str_cmp( argument, "off" ) )
   {
      ship->autorecharge = FALSE;
      send_to_char( "&GYou shutdown the shields.\r\n", ch );
      echo_to_cockpit( AT_YELLOW, ship, "Shields OFF. Shield strength set to 0. Autorecharge OFF." );
      ship->shield = 0;
   }
   else if( !str_cmp( argument, "idle" ) )
   {
      ship->autorecharge = FALSE;
      send_to_char( "&GYou let the shields idle.\r\n", ch );
      echo_to_cockpit( AT_YELLOW, ship, "Autorecharge OFF. Shields IDLEING." );
   }
   else
   {
      if( ship->autorecharge == TRUE )
      {
         ship->autorecharge = FALSE;
         send_to_char( "&GYou toggle the shields.\r\n", ch );
         echo_to_cockpit( AT_YELLOW, ship, "Autorecharge OFF. Shields IDLEING." );
      }
      else
      {
         ship->autorecharge = TRUE;
         send_to_char( "&GYou toggle the shields.\r\n", ch );
         echo_to_cockpit( AT_YELLOW, ship, "Shields ON. Autorecharge ON" );
      }
   }

   if( ship->autorecharge )
   {
      recharge = URANGE( 1, ship->maxshield - ship->shield, 25 + ship->ship_class * 25 );
      recharge = UMIN( recharge, ship->energy * 5 + 100 );
      ship->shield += recharge;
      ship->energy -= ( recharge * 2 + recharge * ship->ship_class );
   }

   learn_from_success( ch, gsn_shipsystems );
}

void do_autopilot( CHAR_DATA * ch, const char *argument )
{
   SHIP_DATA *ship;

   if( ( ship = ship_from_cockpit( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou must be in the cockpit of a ship to do that!\r\n", ch );
      return;
   }

   if( ( ship = ship_from_pilotseat( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou must be in the pilots seat!\r\n", ch );
      return;
   }

   if( !check_pilot( ch, ship ) )
   {
      send_to_char( "&RHey! Thats not your ship!\r\n", ch );
      return;
   }

   if( ship->target0 || ship->target1 || ship->target2 )
   {
      send_to_char( "&RNot while the ship is enganged with an enemy!\r\n", ch );
      return;
   }


   act( AT_PLAIN, "$n flips a switch on the control panell.", ch, NULL, argument, TO_ROOM );

   if( ship->autopilot == TRUE )
   {
      ship->autopilot = FALSE;
      send_to_char( "&GYou toggle the autopilot.\r\n", ch );
      echo_to_cockpit( AT_YELLOW, ship, "Autopilot OFF." );
   }
   else
   {
      ship->autopilot = TRUE;
      ship->autorecharge = TRUE;
      send_to_char( "&GYou toggle the autopilot.\r\n", ch );
      echo_to_cockpit( AT_YELLOW, ship, "Autopilot ON." );
   }

}

void do_openhatch( CHAR_DATA * ch, const char *argument )
{
   SHIP_DATA *ship;
   char buf[MAX_STRING_LENGTH];

   if( !argument || argument[0] == '\0' || !str_cmp( argument, "hatch" ) )
   {
      ship = ship_from_entrance( ch->in_room->vnum );
      if( ship == NULL )
      {
         send_to_char( "&ROpen what?\r\n", ch );
         return;
      }
      else
      {
         if( !ship->hatchopen )
         {

            if( ship->ship_class == SHIP_PLATFORM )
            {
               send_to_char( "&RTry one of the docking bays!\r\n", ch );
               return;
            }
            if( ship->location != ship->lastdoc || ( ship->shipstate != SHIP_DOCKED && ship->shipstate != SHIP_DISABLED ) )
            {
               send_to_char( "&RPlease wait till the ship lands!\r\n", ch );
               return;
            }
            ship->hatchopen = TRUE;
            send_to_char( "&GYou open the hatch.\r\n", ch );
            act( AT_PLAIN, "$n opens the hatch.", ch, NULL, argument, TO_ROOM );
            sprintf( buf, "The hatch on %s opens.", ship->name );
            echo_to_room( AT_YELLOW, get_room_index( ship->location ), buf );
            sound_to_room( get_room_index( ship->entrance ), "!!SOUND(door)" );
            sound_to_room( get_room_index( ship->location ), "!!SOUND(door)" );
            return;
         }
         else
         {
            send_to_char( "&RIt's already open.\r\n", ch );
            return;
         }
      }
   }

   ship = ship_in_room( ch->in_room, argument );
   if( !ship )
   {
      act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
      return;
   }

   if( ship->shipstate != SHIP_DOCKED && ship->shipstate != SHIP_DISABLED )
   {
      send_to_char( "&RThat ship has already started to launch", ch );
      return;
   }

   if( !check_pilot( ch, ship ) )
   {
      send_to_char( "&RHey! Thats not your ship!\r\n", ch );
      return;
   }

   if( !ship->hatchopen )
   {
      ship->hatchopen = TRUE;
      act( AT_PLAIN, "You open the hatch on $T.", ch, NULL, ship->name, TO_CHAR );
      act( AT_PLAIN, "$n opens the hatch on $T.", ch, NULL, ship->name, TO_ROOM );
      echo_to_room( AT_YELLOW, get_room_index( ship->entrance ), "The hatch opens from the outside." );
      sound_to_room( get_room_index( ship->entrance ), "!!SOUND(door)" );
      sound_to_room( get_room_index( ship->location ), "!!SOUND(door)" );
      return;
   }

   send_to_char( "&GIts already open!\r\n", ch );

}


void do_closehatch( CHAR_DATA * ch, const char *argument )
{
   SHIP_DATA *ship;
   char buf[MAX_STRING_LENGTH];

   if( !argument || argument[0] == '\0' || !str_cmp( argument, "hatch" ) )
   {
      ship = ship_from_entrance( ch->in_room->vnum );
      if( ship == NULL )
      {
         send_to_char( "&RClose what?\r\n", ch );
         return;
      }
      else
      {

         if( ship->ship_class == SHIP_PLATFORM )
         {
            send_to_char( "&RTry one of the docking bays!\r\n", ch );
            return;
         }
         if( ship->hatchopen )
         {
            ship->hatchopen = FALSE;
            send_to_char( "&GYou close the hatch.\r\n", ch );
            act( AT_PLAIN, "$n closes the hatch.", ch, NULL, argument, TO_ROOM );
            sprintf( buf, "The hatch on %s closes.", ship->name );
            echo_to_room( AT_YELLOW, get_room_index( ship->location ), buf );
            sound_to_room( get_room_index( ship->entrance ), "!!SOUND(door)" );
            sound_to_room( get_room_index( ship->location ), "!!SOUND(door)" );
            return;
         }
         else
         {
            send_to_char( "&RIt's already closed.\r\n", ch );
            return;
         }
      }
   }

   ship = ship_in_room( ch->in_room, argument );
   if( !ship )
   {
      act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
      return;
   }

   if( ship->shipstate != SHIP_DOCKED && ship->shipstate != SHIP_DISABLED )
   {
      send_to_char( "&RThat ship has already started to launch", ch );
      return;
   }
   else
   {
      if( ship->hatchopen )
      {
         ship->hatchopen = FALSE;
         act( AT_PLAIN, "You close the hatch on $T.", ch, NULL, ship->name, TO_CHAR );
         act( AT_PLAIN, "$n closes the hatch on $T.", ch, NULL, ship->name, TO_ROOM );
         echo_to_room( AT_YELLOW, get_room_index( ship->entrance ), "The hatch is closed from outside." );
         sound_to_room( get_room_index( ship->entrance ), "!!SOUND(door)" );
         sound_to_room( get_room_index( ship->location ), "!!SOUND(door)" );

         return;
      }
      else
      {
         send_to_char( "&RIts already closed.\r\n", ch );
         return;
      }
   }


}

void do_status( CHAR_DATA * ch, const char *argument )
{
   int schance;
   SHIP_DATA *ship;
   SHIP_DATA *target;

   if( ( ship = ship_from_cockpit( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou must be in the cockpit, turret or engineroom of a ship to do that!\r\n", ch );
      return;
   }

   if( argument[0] == '\0' )
      target = ship;
   else
      target = get_ship_here( argument, ship->starsystem );

   if( target == NULL )
   {
      send_to_char( "&RI don't see that here.\r\nTry the radar, or type status by itself for your ships status.\r\n", ch );
      return;
   }

   if( abs( ( int )( target->vx - ship->vx ) ) > 500 + ship->sensor * 2
       || abs( ( int )( target->vy - ship->vy ) ) > 500 + ship->sensor * 2
       || abs( ( int )( target->vz - ship->vz ) ) > 500 + ship->sensor * 2 )
   {
      send_to_char( "&RThat ship is to far away to scan.\r\n", ch );
      return;
   }

   schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_shipsystems] );
   if( number_percent(  ) > schance )
   {
      send_to_char( "&RYou cant figure out what the readout means.\r\n", ch );
      learn_from_failure( ch, gsn_shipsystems );
      return;
   }

   act( AT_PLAIN, "$n checks various gages and displays on the control panel.", ch, NULL, argument, TO_ROOM );

   ch_printf( ch, "&W%s:\r\n", target->name );
   ch_printf( ch, "&OCurrent Coordinates:&Y %.0f %.0f %.0f\r\n", target->vx, target->vy, target->vz );
   ch_printf( ch, "&OCurrent Heading:&Y %.0f %.0f %.0f\r\n", target->hx, target->hy, target->hz );
   ch_printf( ch, "&OCurrent Speed:&Y %d&O/%d\r\n", target->currspeed, target->realspeed );
   ch_printf( ch, "&OHull:&Y %d&O/%d  Ship Condition:&Y %s\r\n",
              target->hull, target->maxhull, target->shipstate == SHIP_DISABLED ? "Disabled" : "Running" );
   ch_printf( ch, "&OShields:&Y %d&O/%d   Energy(fuel):&Y %d&O/%d\r\n",
              target->shield, target->maxshield, target->energy, target->maxenergy );
   ch_printf( ch, "&OLaser Condition:&Y %s  &OCurrent Target:&Y %s\r\n",
              target->statet0 == LASER_DAMAGED ? "Damaged" : "Good", target->target0 ? target->target0->name : "none" );
   if( target->turret1 )
      ch_printf( ch, "&OTurret One:&Y %s  &OCurrent Target:&Y %s\r\n",
                 target->statet1 == LASER_DAMAGED ? "Damaged" : "Good", target->target1 ? target->target1->name : "none" );
   if( target->turret2 )
      ch_printf( ch, "&OTurret Two:&Y %s  &OCurrent Target:&Y %s\r\n",
                 target->statet2 == LASER_DAMAGED ? "Damaged" : "Good", target->target2 ? target->target2->name : "none" );
   ch_printf( ch, "\r\n&OMissiles:&Y %d&O/%d  Torpedos: &Y%d&O/%d  Rockets: &Y%d&O/%d  Condition:&Y %s&w\r\n",
              ship->missiles,
              ship->maxmissiles,
              ship->torpedos,
              ship->maxtorpedos,
              ship->rockets, ship->maxrockets, ship->missilestate == MISSILE_DAMAGED ? "Damaged" : "Good" );

   learn_from_success( ch, gsn_shipsystems );
}

void do_hyperspace( CHAR_DATA * ch, const char *argument )
{
   int schance = 0;
   SHIP_DATA *ship;
   SHIP_DATA *eShip;
   char buf[MAX_STRING_LENGTH];

   if( ( ship = ship_from_cockpit( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou must be in the cockpit of a ship to do that!\r\n", ch );
      return;
   }

   if( ship->ship_class > SHIP_PLATFORM )
   {
      send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
      return;
   }


   if( ( ship = ship_from_pilotseat( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou aren't in the pilots seat.\r\n", ch );
      return;
   }

   if( autofly( ship ) )
   {
      send_to_char( "&RYou'll have to turn off the ships autopilot first.\r\n", ch );
      return;
   }


   if( ship->ship_class == SHIP_PLATFORM )
   {
      send_to_char( "&RPlatforms can't move!\r\n", ch );
      return;
   }
   if( ship->hyperspeed == 0 )
   {
      send_to_char( "&RThis ship is not equipped with a hyperdrive!\r\n", ch );
      return;
   }
   if( ship->shipstate == SHIP_HYPERSPACE )
   {
      send_to_char( "&RYou are already travelling lightspeed!\r\n", ch );
      return;
   }
   if( ship->shipstate == SHIP_DISABLED )
   {
      send_to_char( "&RThe ships drive is disabled. Unable to manuever.\r\n", ch );
      return;
   }
   if( ship->shipstate == SHIP_DOCKED )
   {
      send_to_char( "&RYou can't do that until after you've launched!\r\n", ch );
      return;
   }
   if( ship->shipstate != SHIP_READY )
   {
      send_to_char( "&RPlease wait until the ship has finished its current manouver.\r\n", ch );
      return;
   }
   if( !ship->currjump )
   {
      send_to_char( "&RYou need to calculate your jump first!\r\n", ch );
      return;
   }

   if( ship->energy < ( 200 + ship->hyperdistance * ( 1 + ship->ship_class ) / 3 ) )
   {
      send_to_char( "&RTheres not enough fuel!\r\n", ch );
      return;
   }

   if( ship->currspeed <= 0 )
   {
      send_to_char( "&RYou need to speed up a little first!\r\n", ch );
      return;
   }

   for( eShip = ship->starsystem->first_ship; eShip; eShip = eShip->next_in_starsystem )
   {
      if( eShip == ship )
         continue;

      if( abs( ( int )( eShip->vx - ship->vx ) ) < 500
	  && abs( ( int )( eShip->vy - ship->vy ) ) < 500
	  && abs( ( int )( eShip->vz - ship->vz ) ) < 500 )
      {
         ch_printf( ch, "&RYou are too close to %s to make the jump to lightspeed.\r\n", eShip->name );
         return;
      }
   }

   if( ship->ship_class == FIGHTER_SHIP )
      schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_starfighters] );
   if( ship->ship_class == MIDSIZE_SHIP )
      schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_midships] );
   if( ship->ship_class == CAPITAL_SHIP )
      schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_capitalships] );
   if( number_percent(  ) > schance )
   {
      send_to_char( "&RYou can't figure out which lever to use.\r\n", ch );
      if( ship->ship_class == FIGHTER_SHIP )
         learn_from_failure( ch, gsn_starfighters );
      if( ship->ship_class == MIDSIZE_SHIP )
         learn_from_failure( ch, gsn_midships );
      if( ship->ship_class == CAPITAL_SHIP )
         learn_from_failure( ch, gsn_capitalships );
      return;
   }
   sprintf( buf, "%s disapears from your scanner.", ship->name );
   echo_to_system( AT_YELLOW, ship, buf, NULL );

   ship_from_starsystem( ship, ship->starsystem );
   ship->shipstate = SHIP_HYPERSPACE;

   send_to_char( "&GYou push forward the hyperspeed lever.\r\n", ch );
   act( AT_PLAIN, "$n pushes a lever forward on the control panel.", ch, NULL, argument, TO_ROOM );
   echo_to_ship( AT_YELLOW, ship, "The ship lurches slightly as it makes the jump to lightspeed." );
   echo_to_cockpit( AT_YELLOW, ship, "The stars become streaks of light as you enter hyperspace." );

   ship->energy -= ( 100 + ship->hyperdistance * ( 1 + ship->ship_class ) / 3 );

   ship->vx = ship->jx;
   ship->vy = ship->jy;
   ship->vz = ship->jz;

   if( ship->ship_class == FIGHTER_SHIP )
      learn_from_success( ch, gsn_starfighters );
   if( ship->ship_class == MIDSIZE_SHIP )
      learn_from_success( ch, gsn_midships );
   if( ship->ship_class == CAPITAL_SHIP )
      learn_from_success( ch, gsn_capitalships );

}


void do_target( CHAR_DATA * ch, const char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   int schance;
   SHIP_DATA *ship;
   SHIP_DATA *target;
   char buf[MAX_STRING_LENGTH];

   strcpy( arg, argument );

   switch ( ch->substate )
   {
      default:
         if( ( ship = ship_from_turret( ch->in_room->vnum ) ) == NULL )
         {
            send_to_char( "&RYou must be in the gunners seat or turret of a ship to do that!\r\n", ch );
            return;
         }

         if( ship->ship_class > SHIP_PLATFORM )
         {
            send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
            return;
         }

         if( ship->shipstate == SHIP_HYPERSPACE )
         {
            send_to_char( "&RYou can only do that in realspace!\r\n", ch );
            return;
         }
         if( !ship->starsystem )
         {
            send_to_char( "&RYou can't do that until you've finished launching!\r\n", ch );
            return;
         }

         if( autofly( ship ) )
         {
            send_to_char( "&RYou'll have to turn off the ships autopilot first....\r\n", ch );
            return;
         }

         if( arg[0] == '\0' )
         {
            send_to_char( "&RYou need to specify a target!\r\n", ch );
            return;
         }

         if( !str_cmp( arg, "none" ) )
         {
            send_to_char( "&GTarget set to none.\r\n", ch );
            if( ch->in_room->vnum == ship->gunseat )
               ship->target0 = NULL;
            if( ch->in_room->vnum == ship->turret1 )
               ship->target1 = NULL;
            if( ch->in_room->vnum == ship->turret2 )
               ship->target2 = NULL;
            return;
         }

         target = get_ship_here( arg, ship->starsystem );
         if( target == NULL )
         {
            send_to_char( "&RThat ship isn't here!\r\n", ch );
            return;
         }

         if( target == ship )
         {
            send_to_char( "&RYou can't target your own ship!\r\n", ch );
            return;
         }

         if( !str_cmp( target->owner, ship->owner ) && str_cmp( target->owner, "" ) )
         {
            send_to_char( "&RThat ship has the same owner... try targetting an enemy ship instead!\r\n", ch );
            return;
         }

         if( abs( ( int )( ship->vx - target->vx ) ) > 5000
	     || abs( ( int )( ship->vy - target->vy ) ) > 5000
	     || abs( ( int )( ship->vz - target->vz ) ) > 5000 )
         {
            send_to_char( "&RThat ship is too far away to target.\r\n", ch );
            return;
         }

         schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_weaponsystems] );
         if( number_percent(  ) < schance )
         {
            send_to_char( "&GTracking target.\r\n", ch );
            act( AT_PLAIN, "$n makes some adjustments on the targeting computer.", ch, NULL, argument, TO_ROOM );
            add_timer( ch, TIMER_DO_FUN, 1, do_target, 1 );
            ch->dest_buf = str_dup( arg );
            return;
         }
         send_to_char( "&RYou fail to work the controls properly.\r\n", ch );
         learn_from_failure( ch, gsn_weaponsystems );
         return;

      case 1:
         if( !ch->dest_buf )
            return;
         strcpy( arg, ( const char* ) ch->dest_buf );
         DISPOSE( ch->dest_buf );
         break;

      case SUB_TIMER_DO_ABORT:
         DISPOSE( ch->dest_buf );
         ch->substate = SUB_NONE;
         if( ( ship = ship_from_cockpit( ch->in_room->vnum ) ) == NULL )
            return;
         send_to_char( "&RYour concentration is broken. You fail to lock onto your target.\r\n", ch );
         return;
   }

   ch->substate = SUB_NONE;

   if( ( ship = ship_from_turret( ch->in_room->vnum ) ) == NULL )
   {
      return;
   }

   target = get_ship_here( arg, ship->starsystem );
   if( target == NULL || target == ship )
   {
      send_to_char( "&RThe ship has left the starsytem. Targeting aborted.\r\n", ch );
      return;
   }

   if( ch->in_room->vnum == ship->gunseat )
      ship->target0 = target;

   if( ch->in_room->vnum == ship->turret1 )
      ship->target1 = target;

   if( ch->in_room->vnum == ship->turret2 )
      ship->target2 = target;

   send_to_char( "&GTarget Locked.\r\n", ch );
   sprintf( buf, "You are being targetted by %s.", ship->name );
   echo_to_cockpit( AT_BLOOD, target, buf );

   sound_to_room( ch->in_room, "!!SOUND(targetlock)" );
   learn_from_success( ch, gsn_weaponsystems );

   if( autofly( target ) && !target->target0 )
   {
      sprintf( buf, "You are being targetted by %s.", target->name );
      echo_to_cockpit( AT_BLOOD, ship, buf );
      target->target0 = ship;
   }
}

void do_fire( CHAR_DATA * ch, const char *argument )
{
   int schance;
   SHIP_DATA *ship;
   SHIP_DATA *target;
   char buf[MAX_STRING_LENGTH];

   if( ( ship = ship_from_turret( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou must be in the gunners chair or turret of a ship to do that!\r\n", ch );
      return;
   }

   if( ship->ship_class > SHIP_PLATFORM )
   {
      send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
      return;
   }

   if( ship->shipstate == SHIP_HYPERSPACE )
   {
      send_to_char( "&RYou can only do that in realspace!\r\n", ch );
      return;
   }
   if( ship->starsystem == NULL )
   {
      send_to_char( "&RYou can't do that until after you've finished launching!\r\n", ch );
      return;
   }
   if( ship->energy < 5 )
   {
      send_to_char( "&RTheres not enough energy left to fire!\r\n", ch );
      return;
   }

   if( autofly( ship ) )
   {
      send_to_char( "&RYou'll have to turn off the ships autopilot first.\r\n", ch );
      return;
   }


   schance = IS_NPC( ch ) ? ch->top_level
      : ( int )( ch->perm_dex * 2 + ch->pcdata->learned[gsn_spacecombat] / 3
                 + ch->pcdata->learned[gsn_spacecombat2] / 3 + ch->pcdata->learned[gsn_spacecombat3] / 3 );

   if( ch->in_room->vnum == ship->gunseat && !str_prefix( argument, "lasers" ) )
   {

      if( ship->statet0 == LASER_DAMAGED )
      {
         send_to_char( "&RThe ships main laser is damaged.\r\n", ch );
         return;
      }
      if( ship->statet0 >= ship->lasers )
      {
         send_to_char( "&RThe lasers are still recharging.\r\n", ch );
         return;
      }
      if( ship->target0 == NULL )
      {
         send_to_char( "&RYou need to choose a target first.\r\n", ch );
         return;
      }
      target = ship->target0;
      if( ship->target0->starsystem != ship->starsystem )
      {
         send_to_char( "&RYour target seems to have left.\r\n", ch );
         ship->target0 = NULL;
         return;
      }
      if( abs( ( int )( target->vx - ship->vx ) ) > 1000
	  || abs( ( int )( target->vy - ship->vy ) ) > 1000
	  || abs( ( int )( target->vz - ship->vz ) ) > 1000 )
      {
         send_to_char( "&RThat ship is out of laser range.\r\n", ch );
         return;
      }
      if( ship->ship_class < 2 && !is_facing( ship, target ) )
      {
         send_to_char( "&RThe main laser can only fire forward. You'll need to turn your ship!\r\n", ch );
         return;
      }
      ship->statet0++;
      schance += target->ship_class * 25;
      schance -= target->manuever / 10;
      schance -= target->currspeed / 20;
      schance -= ( abs( ( int )( target->vx - ship->vx ) ) / 70 );
      schance -= ( abs( ( int )( target->vy - ship->vy ) ) / 70 );
      schance -= ( abs( ( int )( target->vz - ship->vz ) ) / 70 );
      schance = URANGE( 10, schance, 90 );
      act( AT_PLAIN, "$n presses the fire button.", ch, NULL, argument, TO_ROOM );
      if( number_percent(  ) > schance )
      {
         sprintf( buf, "Lasers fire from %s at you but miss.", ship->name );
         echo_to_cockpit( AT_ORANGE, target, buf );
         sprintf( buf, "The ships lasers fire at %s but miss.", target->name );
         echo_to_cockpit( AT_ORANGE, ship, buf );
         learn_from_failure( ch, gsn_spacecombat );
         learn_from_failure( ch, gsn_spacecombat2 );
         learn_from_failure( ch, gsn_spacecombat3 );
         sprintf( buf, "Laserfire from %s barely misses %s.", ship->name, target->name );
         echo_to_system( AT_ORANGE, ship, buf, target );
         return;
      }
      sprintf( buf, "Laserfire from %s hits %s.", ship->name, target->name );
      echo_to_system( AT_ORANGE, ship, buf, target );
      sprintf( buf, "You are hit by lasers from %s!", ship->name );
      echo_to_cockpit( AT_BLOOD, target, buf );
      sprintf( buf, "Your ships lasers hit %s!.", target->name );
      echo_to_cockpit( AT_YELLOW, ship, buf );
      learn_from_success( ch, gsn_spacecombat );
      learn_from_success( ch, gsn_spacecombat2 );
      learn_from_success( ch, gsn_spacecombat3 );
      echo_to_ship( AT_RED, target, "A small explosion vibrates through the ship." );
      damage_ship_ch( target, 5, 10, ch );

      if( autofly( target ) && target->target0 != ship )
      {
         target->target0 = ship;
         sprintf( buf, "You are being targetted by %s.", target->name );
         echo_to_cockpit( AT_BLOOD, ship, buf );
      }

      return;
   }

   if( ch->in_room->vnum == ship->gunseat && !str_prefix( argument, "missile" ) )
   {
      if( ship->missilestate == MISSILE_DAMAGED )
      {
         send_to_char( "&RThe ships missile launchers are dammaged.\r\n", ch );
         return;
      }
      if( ship->missiles <= 0 )
      {
         send_to_char( "&RYou have no missiles to fire!\r\n", ch );
         return;
      }
      if( ship->missilestate != MISSILE_READY )
      {
         send_to_char( "&RThe missiles are still reloading.\r\n", ch );
         return;
      }
      if( ship->target0 == NULL )
      {
         send_to_char( "&RYou need to choose a target first.\r\n", ch );
         return;
      }
      target = ship->target0;
      if( ship->target0->starsystem != ship->starsystem )
      {
         send_to_char( "&RYour target seems to have left.\r\n", ch );
         ship->target0 = NULL;
         return;
      }
      if( abs( ( int )( target->vx - ship->vx ) ) > 1000
	  || abs( ( int )( target->vy - ship->vy ) ) > 1000
	  || abs( ( int )( target->vz - ship->vz ) ) > 1000 )
      {
         send_to_char( "&RThat ship is out of missile range.\r\n", ch );
         return;
      }
      if( ship->ship_class < 2 && !is_facing( ship, target ) )
      {
         send_to_char( "&RMissiles can only fire in a forward. You'll need to turn your ship!\r\n", ch );
         return;
      }
      schance -= target->manuever / 5;
      schance -= target->currspeed / 20;
      schance += target->ship_class * target->ship_class * 25;
      schance -= ( abs( ( int )( target->vx - ship->vx ) ) / 100 );
      schance -= ( abs( ( int )( target->vy - ship->vy ) ) / 100 );
      schance -= ( abs( ( int )( target->vz - ship->vz ) ) / 100 );
      schance += ( 30 );
      schance = URANGE( 20, schance, 80 );
      act( AT_PLAIN, "$n presses the fire button.", ch, NULL, argument, TO_ROOM );
      if( number_percent(  ) > schance )
      {
         send_to_char( "&RYou fail to lock onto your target!", ch );
         ship->missilestate = MISSILE_RELOAD_2;
         return;
      }
      new_missile( ship, target, ch, CONCUSSION_MISSILE );
      ship->missiles--;
      act( AT_PLAIN, "$n presses the fire button.", ch, NULL, argument, TO_ROOM );
      echo_to_cockpit( AT_YELLOW, ship, "Missiles launched." );
      sprintf( buf, "Incoming missile from %s.", ship->name );
      echo_to_cockpit( AT_BLOOD, target, buf );
      sprintf( buf, "%s fires a missile towards %s.", ship->name, target->name );
      echo_to_system( AT_ORANGE, ship, buf, target );
      learn_from_success( ch, gsn_weaponsystems );
      if( ship->ship_class == CAPITAL_SHIP || ship->ship_class == SHIP_PLATFORM )
         ship->missilestate = MISSILE_RELOAD;
      else
         ship->missilestate = MISSILE_FIRED;

      if( autofly( target ) && target->target0 != ship )
      {
         target->target0 = ship;
         sprintf( buf, "You are being targetted by %s.", target->name );
         echo_to_cockpit( AT_BLOOD, ship, buf );
      }

      return;
   }
   if( ch->in_room->vnum == ship->gunseat && !str_prefix( argument, "torpedo" ) )
   {
      if( ship->missilestate == MISSILE_DAMAGED )
      {
         send_to_char( "&RThe ships missile launchers are dammaged.\r\n", ch );
         return;
      }
      if( ship->torpedos <= 0 )
      {
         send_to_char( "&RYou have no torpedos to fire!\r\n", ch );
         return;
      }
      if( ship->missilestate != MISSILE_READY )
      {
         send_to_char( "&RThe torpedos are still reloading.\r\n", ch );
         return;
      }
      if( ship->target0 == NULL )
      {
         send_to_char( "&RYou need to choose a target first.\r\n", ch );
         return;
      }
      target = ship->target0;
      if( ship->target0->starsystem != ship->starsystem )
      {
         send_to_char( "&RYour target seems to have left.\r\n", ch );
         ship->target0 = NULL;
         return;
      }
      if( abs( ( int )( target->vx - ship->vx ) ) > 1000
	  || abs( ( int )( target->vy - ship->vy ) ) > 1000
	  || abs( ( int )( target->vz - ship->vz ) ) > 1000 )
      {
         send_to_char( "&RThat ship is out of torpedo range.\r\n", ch );
         return;
      }
      if( ship->ship_class < 2 && !is_facing( ship, target ) )
      {
         send_to_char( "&RTorpedos can only fire in a forward direction. You'll need to turn your ship!\r\n", ch );
         return;
      }
      schance -= target->manuever / 5;
      schance -= target->currspeed / 20;
      schance += target->ship_class * target->ship_class * 25;
      schance -= ( abs( ( int )( target->vx - ship->vx ) ) / 100 );
      schance -= ( abs( ( int )( target->vy - ship->vy ) ) / 100 );
      schance -= ( abs( ( int )( target->vz - ship->vz ) ) / 100 );
      schance = URANGE( 20, schance, 80 );
      act( AT_PLAIN, "$n presses the fire button.", ch, NULL, argument, TO_ROOM );
      if( number_percent(  ) > schance )
      {
         send_to_char( "&RYou fail to lock onto your target!", ch );
         ship->missilestate = MISSILE_RELOAD_2;
         return;
      }
      new_missile( ship, target, ch, PROTON_TORPEDO );
      ship->torpedos--;
      act( AT_PLAIN, "$n presses the fire button.", ch, NULL, argument, TO_ROOM );
      echo_to_cockpit( AT_YELLOW, ship, "Missiles launched." );
      sprintf( buf, "Incoming torpedo from %s.", ship->name );
      echo_to_cockpit( AT_BLOOD, target, buf );
      sprintf( buf, "%s fires a torpedo towards %s.", ship->name, target->name );
      echo_to_system( AT_ORANGE, ship, buf, target );
      learn_from_success( ch, gsn_weaponsystems );
      if( ship->ship_class == CAPITAL_SHIP || ship->ship_class == SHIP_PLATFORM )
         ship->missilestate = MISSILE_RELOAD;
      else
         ship->missilestate = MISSILE_FIRED;

      if( autofly( target ) && target->target0 != ship )
      {
         target->target0 = ship;
         sprintf( buf, "You are being targetted by %s.", target->name );
         echo_to_cockpit( AT_BLOOD, ship, buf );
      }

      return;
   }

   if( ch->in_room->vnum == ship->gunseat && !str_prefix( argument, "rocket" ) )
   {
      if( ship->missilestate == MISSILE_DAMAGED )
      {
         send_to_char( "&RThe ships missile launchers are damaged.\r\n", ch );
         return;
      }
      if( ship->rockets <= 0 )
      {
         send_to_char( "&RYou have no rockets to fire!\r\n", ch );
         return;
      }
      if( ship->missilestate != MISSILE_READY )
      {
         send_to_char( "&RThe missiles are still reloading.\r\n", ch );
         return;
      }
      if( ship->target0 == NULL )
      {
         send_to_char( "&RYou need to choose a target first.\r\n", ch );
         return;
      }
      target = ship->target0;
      if( ship->target0->starsystem != ship->starsystem )
      {
         send_to_char( "&RYour target seems to have left.\r\n", ch );
         ship->target0 = NULL;
         return;
      }
      if( abs( ( int )( target->vx - ship->vx ) ) > 800
	  || abs( ( int )( target->vy - ship->vy ) ) > 800
	  || abs( ( int )( target->vz - ship->vz ) ) > 800 )
      {
         send_to_char( "&RThat ship is out of rocket range.\r\n", ch );
         return;
      }
      if( ship->ship_class < 2 && !is_facing( ship, target ) )
      {
         send_to_char( "&RRockets can only fire forward. You'll need to turn your ship!\r\n", ch );
         return;
      }
      schance -= target->manuever / 5;
      schance -= target->currspeed / 20;
      schance += target->ship_class * target->ship_class * 25;
      schance -= ( abs( ( int )( target->vx - ship->vx ) ) / 100 );
      schance -= ( abs( ( int )( target->vy - ship->vy ) ) / 100 );
      schance -= ( abs( ( int )( target->vz - ship->vz ) ) / 100 );
      schance -= 30;
      schance = URANGE( 20, schance, 80 );
      act( AT_PLAIN, "$n presses the fire button.", ch, NULL, argument, TO_ROOM );
      if( number_percent(  ) > schance )
      {
         send_to_char( "&RYou fail to lock onto your target!", ch );
         ship->missilestate = MISSILE_RELOAD_2;
         return;
      }
      new_missile( ship, target, ch, HEAVY_ROCKET );
      ship->rockets--;
      act( AT_PLAIN, "$n presses the fire button.", ch, NULL, argument, TO_ROOM );
      echo_to_cockpit( AT_YELLOW, ship, "Rocket launched." );
      sprintf( buf, "Incoming rocket from %s.", ship->name );
      echo_to_cockpit( AT_BLOOD, target, buf );
      sprintf( buf, "%s fires a heavy rocket towards %s.", ship->name, target->name );
      echo_to_system( AT_ORANGE, ship, buf, target );
      learn_from_success( ch, gsn_weaponsystems );
      if( ship->ship_class == CAPITAL_SHIP || ship->ship_class == SHIP_PLATFORM )
         ship->missilestate = MISSILE_RELOAD;
      else
         ship->missilestate = MISSILE_FIRED;

      if( autofly( target ) && target->target0 != ship )
      {
         target->target0 = ship;
         sprintf( buf, "You are being targetted by %s.", target->name );
         echo_to_cockpit( AT_BLOOD, ship, buf );
      }

      return;
   }

   if( ch->in_room->vnum == ship->turret1 && !str_prefix( argument, "lasers" ) )
   {
      if( ship->statet1 == LASER_DAMAGED )
      {
         send_to_char( "&RThe ships turret is damaged.\r\n", ch );
         return;
      }
      if( ship->statet1 > ship->ship_class )
      {
         send_to_char( "&RThe turbolaser is recharging.\r\n", ch );
         return;
      }
      if( ship->target1 == NULL )
      {
         send_to_char( "&RYou need to choose a target first.\r\n", ch );
         return;
      }
      target = ship->target1;
      if( ship->target1->starsystem != ship->starsystem )
      {
         send_to_char( "&RYour target seems to have left.\r\n", ch );
         ship->target1 = NULL;
         return;
      }
      if( abs( ( int )( target->vx - ship->vx ) ) > 1000
	  || abs( ( int )( target->vy - ship->vy ) ) > 1000
	  || abs( ( int )( target->vz - ship->vz ) ) > 1000 )
      {
         send_to_char( "&RThat ship is out of laser range.\r\n", ch );
         return;
      }
      ship->statet1++;
      schance -= target->manuever / 10;
      schance += target->ship_class * 25;
      schance -= target->currspeed / 20;
      schance -= ( abs( ( int )( target->vx - ship->vx ) ) / 70 );
      schance -= ( abs( ( int )( target->vy - ship->vy ) ) / 70 );
      schance -= ( abs( ( int )( target->vz - ship->vz ) ) / 70 );
      schance = URANGE( 10, schance, 90 );
      act( AT_PLAIN, "$n presses the fire button.", ch, NULL, argument, TO_ROOM );
      if( number_percent(  ) > schance )
      {
         sprintf( buf, "Turbolasers fire from %s at you but miss.", ship->name );
         echo_to_cockpit( AT_ORANGE, target, buf );
         sprintf( buf, "Turbolasers fire from the ships turret at %s but miss.", target->name );
         echo_to_cockpit( AT_ORANGE, ship, buf );
         sprintf( buf, "%s fires at %s but misses.", ship->name, target->name );
         echo_to_system( AT_ORANGE, ship, buf, target );
         learn_from_failure( ch, gsn_spacecombat );
         learn_from_failure( ch, gsn_spacecombat2 );
         learn_from_failure( ch, gsn_spacecombat3 );
         return;
      }
      sprintf( buf, "Turboasers fire from %s, hitting %s.", ship->name, target->name );
      echo_to_system( AT_ORANGE, ship, buf, target );
      sprintf( buf, "You are hit by turbolasers from %s!", ship->name );
      echo_to_cockpit( AT_BLOOD, target, buf );
      sprintf( buf, "Turbolasers fire from the turret, hitting %s!.", target->name );
      echo_to_cockpit( AT_YELLOW, ship, buf );
      learn_from_success( ch, gsn_spacecombat );
      learn_from_success( ch, gsn_spacecombat2 );
      learn_from_success( ch, gsn_spacecombat3 );
      echo_to_ship( AT_RED, target, "A small explosion vibrates through the ship." );
      damage_ship_ch( target, 10, 25, ch );

      if( autofly( target ) && target->target0 != ship )
      {
         target->target0 = ship;
         sprintf( buf, "You are being targetted by %s.", target->name );
         echo_to_cockpit( AT_BLOOD, ship, buf );
      }

      return;
   }

   if( ch->in_room->vnum == ship->turret2 && !str_prefix( argument, "lasers" ) )
   {
      if( ship->statet2 == LASER_DAMAGED )
      {
         send_to_char( "&RThe ships turret is damaged.\r\n", ch );
         return;
      }
      if( ship->statet2 > ship->ship_class )
      {
         send_to_char( "&RThe turbolaser is still recharging.\r\n", ch );
         return;
      }
      if( ship->target2 == NULL )
      {
         send_to_char( "&RYou need to choose a target first.\r\n", ch );
         return;
      }
      target = ship->target2;
      if( ship->target2->starsystem != ship->starsystem )
      {
         send_to_char( "&RYour target seems to have left.\r\n", ch );
         ship->target2 = NULL;
         return;
      }
      if( abs( ( int )( target->vx - ship->vx ) ) > 1000
	  || abs( ( int )( target->vy - ship->vy ) ) > 1000
	  || abs( ( int )( target->vz - ship->vz ) ) > 1000 )
      {
         send_to_char( "&RThat ship is out of laser range.\r\n", ch );
         return;
      }
      ship->statet2++;
      schance -= target->manuever / 10;
      schance += target->ship_class * 25;
      schance -= target->currspeed / 20;
      schance -= ( abs( ( int )( target->vx - ship->vx ) ) / 70 );
      schance -= ( abs( ( int )( target->vy - ship->vy ) ) / 70 );
      schance -= ( abs( ( int )( target->vz - ship->vz ) ) / 70 );
      schance = URANGE( 10, schance, 90 );
      act( AT_PLAIN, "$n presses the fire button.", ch, NULL, argument, TO_ROOM );
      if( number_percent(  ) > schance )
      {
         sprintf( buf, "Turbolasers fire from %s barely missing %s.", ship->name, target->name );
         echo_to_system( AT_ORANGE, ship, buf, target );
         sprintf( buf, "Turbolasers fire from %s at you but miss.", ship->name );
         echo_to_cockpit( AT_ORANGE, target, buf );
         sprintf( buf, "Turbolasers fire from the turret missing %s.", target->name );
         echo_to_cockpit( AT_ORANGE, ship, buf );
         learn_from_failure( ch, gsn_spacecombat );
         learn_from_failure( ch, gsn_spacecombat2 );
         learn_from_failure( ch, gsn_spacecombat3 );
         return;
      }
      sprintf( buf, "Turbolasers fire from %s, hitting %s.", ship->name, target->name );
      echo_to_system( AT_ORANGE, ship, buf, target );
      sprintf( buf, "You are hit by turbolasers from %s!", ship->name );
      echo_to_cockpit( AT_BLOOD, target, buf );
      sprintf( buf, "turbolasers fire from the turret hitting %s!.", target->name );
      echo_to_cockpit( AT_YELLOW, ship, buf );
      learn_from_success( ch, gsn_spacecombat );
      learn_from_success( ch, gsn_spacecombat2 );
      learn_from_success( ch, gsn_spacecombat3 );
      echo_to_ship( AT_RED, target, "A small explosion vibrates through the ship." );
      damage_ship_ch( target, 10, 25, ch );

      if( autofly( target ) && target->target0 != ship )
      {
         target->target0 = ship;
         sprintf( buf, "You are being targetted by %s.", target->name );
         echo_to_cockpit( AT_BLOOD, ship, buf );
      }

      return;
   }

   send_to_char( "&RYou can't fire that!\r\n", ch );

}


void do_calculate( CHAR_DATA * ch, const char *argument )
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   char arg3[MAX_INPUT_LENGTH];
   int schance, count = 0;
   SHIP_DATA *ship;
   SPACE_DATA *starsystem;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );


   if( ( ship = ship_from_cockpit( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou must be in the cockpit of a ship to do that!\r\n", ch );
      return;
   }

   if( ship->ship_class > SHIP_PLATFORM )
   {
      send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
      return;
   }

   if( ( ship = ship_from_navseat( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou must be at a nav computer to calculate jumps.\r\n", ch );
      return;
   }

   if( autofly( ship ) )
   {
      send_to_char( "&RYou'll have to turn off the ships autopilot first....\r\n", ch );
      return;
   }

   if( ship->ship_class == SHIP_PLATFORM )
   {
      send_to_char( "&RAnd what exactly are you going to calculate...?\r\n", ch );
      return;
   }
   if( ship->hyperspeed == 0 )
   {
      send_to_char( "&RThis ship is not equipped with a hyperdrive!\r\n", ch );
      return;
   }
   if( ship->shipstate == SHIP_DOCKED )
   {
      send_to_char( "&RYou can't do that until after you've launched!\r\n", ch );
      return;
   }
   if( ship->starsystem == NULL )
   {
      send_to_char( "&RYou can only do that in realspace.\r\n", ch );
      return;
   }
   if( argument[0] == '\0' )
   {
      send_to_char( "&WFormat: Calculate <starsystem> <entry x> <entry y> <entry z>\r\n&wPossible destinations:\r\n", ch );
      for( starsystem = first_starsystem; starsystem; starsystem = starsystem->next )
      {
         set_char_color( AT_NOTE, ch );
         ch_printf( ch, "%-30s %d\r\n", starsystem->name,
                    ( abs( starsystem->xpos - ship->starsystem->xpos ) +
                      abs( starsystem->ypos - ship->starsystem->ypos ) ) / 2 );
         count++;
      }
      if( !count )
      {
         send_to_char( "No Starsystems found.\r\n", ch );
      }
      return;
   }
   schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_navigation] );
   if( number_percent(  ) > schance )
   {
      send_to_char( "&RYou cant seem to figure the charts out today.\r\n", ch );
      learn_from_failure( ch, gsn_navigation );
      return;
   }


   ship->currjump = starsystem_from_name( arg1 );
   ship->jx = atoi( arg2 );
   ship->jy = atoi( arg3 );
   ship->jz = atoi( argument );

   if( ship->currjump == NULL )
   {
      send_to_char( "&RYou can't seem to find that starsytem on your charts.\r\n", ch );
      return;
   }
   else
   {
      SPACE_DATA *starsystem2;

      starsystem2 = ship->currjump;

      if( starsystem2->star1 && strcmp( starsystem2->star1, "" )
	  && abs( ( int )( ship->jx - starsystem2->s1x ) ) < 300
	  && abs( ( int )( ship->jy - starsystem2->s1y ) ) < 300
	  && abs( ( int )( ship->jz - starsystem2->s1z ) ) < 300 )
      {
         echo_to_cockpit( AT_RED, ship, "WARNING.. Jump coordinates too close to stellar object." );
         echo_to_cockpit( AT_RED, ship, "WARNING.. Hyperjump NOT set." );
         ship->currjump = NULL;
         return;
      }
      else if( starsystem2->star2 && strcmp( starsystem2->star2, "" )
	       && abs( ( int )( ship->jx - starsystem2->s2x ) ) < 300
	       && abs( ( int )( ship->jy - starsystem2->s2y ) ) < 300
	       && abs( ( int )( ship->jz - starsystem2->s2z ) ) < 300 )
      {
         echo_to_cockpit( AT_RED, ship, "WARNING.. Jump coordinates too close to stellar object." );
         echo_to_cockpit( AT_RED, ship, "WARNING.. Hyperjump NOT set." );
         ship->currjump = NULL;
         return;
      }
      else if( starsystem2->planet1 && strcmp( starsystem2->planet1, "" )
	       && abs( ( int )( ship->jx - starsystem2->p1x ) ) < 300
	       && abs( ( int )( ship->jy - starsystem2->p1y ) ) < 300
	       && abs( ( int )( ship->jz - starsystem2->p1z ) ) < 300 )
      {
         echo_to_cockpit( AT_RED, ship, "WARNING.. Jump coordinates too close to stellar object." );
         echo_to_cockpit( AT_RED, ship, "WARNING.. Hyperjump NOT set." );
         ship->currjump = NULL;
         return;
      }
      else if( starsystem2->planet2 && strcmp( starsystem2->planet2, "" )
	       && abs( ( int )( ship->jx - starsystem2->p2x ) ) < 300
	       && abs( ( int )( ship->jy - starsystem2->p2y ) ) < 300
	       && abs( ( int )( ship->jz - starsystem2->p2z ) ) < 300 )
      {
         echo_to_cockpit( AT_RED, ship, "WARNING.. Jump coordinates too close to stellar object." );
         echo_to_cockpit( AT_RED, ship, "WARNING.. Hyperjump NOT set." );
         ship->currjump = NULL;
         return;
      }
      else if( starsystem2->planet3 && strcmp( starsystem2->planet3, "" )
	       && abs( ( int )( ship->jx - starsystem2->p3x ) ) < 300
	       && abs( ( int )( ship->jy - starsystem2->p3y ) ) < 300
	       && abs( ( int )( ship->jz - starsystem2->p3z ) ) < 300 )
      {
         echo_to_cockpit( AT_RED, ship, "WARNING.. Jump coordinates too close to stellar object." );
         echo_to_cockpit( AT_RED, ship, "WARNING.. Hyperjump NOT set." );
         ship->currjump = NULL;
         return;
      }
      else
      {
         ship->jx += number_range( -250, 250 );
         ship->jy += number_range( -250, 250 );
         ship->jz += number_range( -250, 250 );
      }
   }

   ship->hyperdistance = abs( ship->starsystem->xpos - ship->currjump->xpos );
   ship->hyperdistance += abs( ship->starsystem->ypos - ship->currjump->ypos );
   ship->hyperdistance /= 5;

   if( ship->hyperdistance < 100 )
      ship->hyperdistance = 100;

   ship->hyperdistance += number_range( 0, 200 );

   sound_to_room( ch->in_room, "!!SOUND(computer)" );

   send_to_char( "&GHyperspace course set. Ready for the jump to lightspeed.\r\n", ch );
   act( AT_PLAIN, "$n does some calculations using the ships computer.", ch, NULL, argument, TO_ROOM );

   learn_from_success( ch, gsn_navigation );

   WAIT_STATE( ch, 2 * PULSE_VIOLENCE );
}

void do_recharge( CHAR_DATA * ch, const char *argument )
{
   int recharge;
   int schance;
   SHIP_DATA *ship;


   if( ( ship = ship_from_cockpit( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou must be in the cockpit of a ship to do that!\r\n", ch );
      return;
   }
   if( ( ship = ship_from_coseat( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RThe controls must be at the co-pilot station.\r\n", ch );
      return;
   }

   if( autofly( ship ) )
   {
      send_to_char( "&R...\r\n", ch );
      return;
   }

   if( ship->shipstate == SHIP_DISABLED )
   {
      send_to_char( "&RThe ships drive is disabled. Unable to manuever.\r\n", ch );
      return;
   }

   if( ship->energy < 100 )
   {
      send_to_char( "&RTheres not enough energy!\r\n", ch );
      return;
   }

   schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_shipsystems] );
   if( number_percent(  ) > schance )
   {
      send_to_char( "&RYou fail to work the controls properly.\r\n", ch );
      learn_from_failure( ch, gsn_shipsystems );
      return;
   }

   send_to_char( "&GRecharging shields..\r\n", ch );
   act( AT_PLAIN, "$n pulls back a lever on the control panel.", ch, NULL, argument, TO_ROOM );

   learn_from_success( ch, gsn_shipsystems );

   recharge = UMIN( ship->maxshield - ship->shield, ship->energy * 5 + 100 );
   recharge = URANGE( 1, recharge, 25 + ship->ship_class * 25 );
   ship->shield += recharge;
   ship->energy -= ( recharge * 2 + recharge * ship->ship_class );
}


void do_repairship( CHAR_DATA * ch, const char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   int schance, change;
   SHIP_DATA *ship;

   strcpy( arg, argument );

   switch ( ch->substate )
   {
      default:
         if( ( ship = ship_from_engine( ch->in_room->vnum ) ) == NULL )
         {
            send_to_char( "&RYou must be in the engine room of a ship to do that!\r\n", ch );
            return;
         }

         if( str_cmp( argument, "hull" ) && str_cmp( argument, "drive" ) &&
             str_cmp( argument, "launcher" ) && str_cmp( argument, "laser" ) &&
             str_cmp( argument, "turret 1" ) && str_cmp( argument, "turret 2" ) )
         {
            send_to_char( "&RYou need to spceify something to repair:\r\n", ch );
            send_to_char( "&rTry: hull, drive, launcher, laser, turret 1, or turret 2\r\n", ch );
            return;
         }

         schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_shipmaintenance] );
         if( number_percent(  ) < schance )
         {
            send_to_char( "&GYou begin your repairs\r\n", ch );
            act( AT_PLAIN, "$n begins repairing the ships $T.", ch, NULL, argument, TO_ROOM );
            if( !str_cmp( arg, "hull" ) )
               add_timer( ch, TIMER_DO_FUN, 15, do_repairship, 1 );
            else
               add_timer( ch, TIMER_DO_FUN, 5, do_repairship, 1 );
            ch->dest_buf = str_dup( arg );
            return;
         }
         send_to_char( "&RYou fail to locate the source of the problem.\r\n", ch );
         learn_from_failure( ch, gsn_shipmaintenance );
         return;

      case 1:
         if( !ch->dest_buf )
            return;
         strcpy( arg, ( const char* ) ch->dest_buf );
         DISPOSE( ch->dest_buf );
         break;

      case SUB_TIMER_DO_ABORT:
         DISPOSE( ch->dest_buf );
         ch->substate = SUB_NONE;
         if( ( ship = ship_from_cockpit( ch->in_room->vnum ) ) == NULL )
            return;
         send_to_char( "&RYou are distracted and fail to finish your repairs.\r\n", ch );
         return;
   }

   ch->substate = SUB_NONE;

   if( ( ship = ship_from_engine( ch->in_room->vnum ) ) == NULL )
   {
      return;
   }

   if( !str_cmp( arg, "hull" ) )
   {
      change = URANGE( 0,
                       number_range( ( int )( ch->pcdata->learned[gsn_shipmaintenance] / 2 ),
                                     ( int )( ch->pcdata->learned[gsn_shipmaintenance] ) ), ( ship->maxhull - ship->hull ) );
      ship->hull += change;
      ch_printf( ch, "&GRepair complete.. Hull strength inreased by %d points.\r\n", change );
   }

   if( !str_cmp( arg, "drive" ) )
   {
      if( ship->location == ship->lastdoc )
         ship->shipstate = SHIP_DOCKED;
      else
         ship->shipstate = SHIP_READY;
      send_to_char( "&GShips drive repaired.\r\n", ch );
   }

   if( !str_cmp( arg, "launcher" ) )
   {
      ship->missilestate = MISSILE_READY;
      send_to_char( "&GMissile launcher repaired.\r\n", ch );
   }

   if( !str_cmp( arg, "laser" ) )
   {
      ship->statet0 = LASER_READY;
      send_to_char( "&GMain laser repaired.\r\n", ch );
   }

   if( !str_cmp( arg, "turret 1" ) )
   {
      ship->statet1 = LASER_READY;
      send_to_char( "&GLaser Turret 1 repaired.\r\n", ch );
   }

   if( !str_cmp( arg, "turret 2" ) )
   {
      ship->statet2 = LASER_READY;
      send_to_char( "&Laser Turret 2 repaired.\r\n", ch );
   }

   act( AT_PLAIN, "$n finishes the repairs.", ch, NULL, argument, TO_ROOM );

   learn_from_success( ch, gsn_shipmaintenance );

}


void do_refuel( CHAR_DATA * ch, const char *argument )
{
}

void do_addpilot( CHAR_DATA * ch, const char *argument )
{
   SHIP_DATA *ship;

   if( ( ship = ship_from_cockpit( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou must be in the cockpit of a ship to do that!\r\n", ch );
      return;
   }

   if( ship->ship_class == SHIP_PLATFORM )
   {
      send_to_char( "&RYou can't do that here.\r\n", ch );
      return;
   }

   if( str_cmp( ship->owner, ch->name ) )
   {

      if( !IS_NPC( ch ) && ch->pcdata && ch->pcdata->clan && !str_cmp( ch->pcdata->clan->name, ship->owner ) )
         if( !str_cmp( ch->pcdata->clan->leader, ch->name ) )
            ;
         else if( !str_cmp( ch->pcdata->clan->number1, ch->name ) )
            ;
         else if( !str_cmp( ch->pcdata->clan->number2, ch->name ) )
            ;
         else
         {
            send_to_char( "&RThat isn't your ship!", ch );
            return;
         }
      else
      {
         send_to_char( "&RThat isn't your ship!", ch );
         return;
      }

   }

   if( argument[0] == '\0' )
   {
      send_to_char( "&RAdd which pilot?\r\n", ch );
      return;
   }

   if( str_cmp( ship->pilot, "" ) )
   {
      if( str_cmp( ship->copilot, "" ) )
      {
         send_to_char( "&RYou are ready have a pilot and copilot..\r\n", ch );
         send_to_char( "&RTry rempilot first.\r\n", ch );
         return;
      }

      STRFREE( ship->copilot );
      ship->copilot = STRALLOC( argument );
      send_to_char( "Copilot Added.\r\n", ch );
      save_ship( ship );
      return;

      return;
   }

   STRFREE( ship->pilot );
   ship->pilot = STRALLOC( argument );
   send_to_char( "Pilot Added.\r\n", ch );
   save_ship( ship );

}

void do_rempilot( CHAR_DATA * ch, const char *argument )
{
   SHIP_DATA *ship;

   if( ( ship = ship_from_cockpit( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou must be in the cockpit of a ship to do that!\r\n", ch );
      return;
   }

   if( ship->ship_class == SHIP_PLATFORM )
   {
      send_to_char( "&RYou can't do that here.\r\n", ch );
      return;
   }

   if( str_cmp( ship->owner, ch->name ) )
   {

      if( !IS_NPC( ch ) && ch->pcdata && ch->pcdata->clan && !str_cmp( ch->pcdata->clan->name, ship->owner ) )
         if( !str_cmp( ch->pcdata->clan->leader, ch->name ) )
            ;
         else if( !str_cmp( ch->pcdata->clan->number1, ch->name ) )
            ;
         else if( !str_cmp( ch->pcdata->clan->number2, ch->name ) )
            ;
         else
         {
            send_to_char( "&RThat isn't your ship!", ch );
            return;
         }
      else
      {
         send_to_char( "&RThat isn't your ship!", ch );
         return;
      }

   }

   if( argument[0] == '\0' )
   {
      send_to_char( "&RRemove which pilot?\r\n", ch );
      return;
   }

   if( !str_cmp( ship->pilot, argument ) )
   {
      STRFREE( ship->pilot );
      ship->pilot = STRALLOC( "" );
      send_to_char( "Pilot Removed.\r\n", ch );
      save_ship( ship );
      return;
   }

   if( !str_cmp( ship->copilot, argument ) )
   {
      STRFREE( ship->copilot );
      ship->copilot = STRALLOC( "" );
      send_to_char( "Copilot Removed.\r\n", ch );
      save_ship( ship );
      return;
   }

   send_to_char( "&RThat person isn't listed as one of the ships pilots.\r\n", ch );

}

void do_radar( CHAR_DATA * ch, const char *argument )
{
   SHIP_DATA *target;
   int schance;
   SHIP_DATA *ship;
   MISSILE_DATA *missile;

   if( ( ship = ship_from_cockpit( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou must be in the cockpit or turret of a ship to do that!\r\n", ch );
      return;
   }

   if( ship->ship_class > SHIP_PLATFORM )
   {
      send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
      return;
   }

   if( ship->shipstate == SHIP_DOCKED )
   {
      send_to_char( "&RWait until after you launch!\r\n", ch );
      return;
   }

   if( ship->shipstate == SHIP_HYPERSPACE )
   {
      send_to_char( "&RYou can only do that in realspace!\r\n", ch );
      return;
   }

   if( ship->starsystem == NULL )
   {
      send_to_char( "&RYou can't do that unless the ship is flying in realspace!\r\n", ch );
      return;
   }

   schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_navigation] );
   if( number_percent(  ) > schance )
   {
      send_to_char( "&RYou fail to work the controls properly.\r\n", ch );
      learn_from_failure( ch, gsn_navigation );
      return;
   }


   act( AT_PLAIN, "$n checks the radar.", ch, NULL, argument, TO_ROOM );

   set_char_color( AT_WHITE, ch );
   ch_printf( ch, "%s\r\n\r\n", ship->starsystem->name );
   set_char_color( AT_LBLUE, ch );
   if( ship->starsystem->star1 && str_cmp( ship->starsystem->star1, "" ) )
      ch_printf( ch, "%s   %d %d %d\r\n",
                 ship->starsystem->star1, ship->starsystem->s1x, ship->starsystem->s1y, ship->starsystem->s1z );
   if( ship->starsystem->star2 && str_cmp( ship->starsystem->star2, "" ) )
      ch_printf( ch, "%s   %d %d %d\r\n",
                 ship->starsystem->star2, ship->starsystem->s2x, ship->starsystem->s2y, ship->starsystem->s2z );
   if( ship->starsystem->planet1 && str_cmp( ship->starsystem->planet1, "" ) )
      ch_printf( ch, "%s   %d %d %d\r\n",
                 ship->starsystem->planet1, ship->starsystem->p1x, ship->starsystem->p1y, ship->starsystem->p1z );
   if( ship->starsystem->planet2 && str_cmp( ship->starsystem->planet2, "" ) )
      ch_printf( ch, "%s   %d %d %d\r\n",
                 ship->starsystem->planet2, ship->starsystem->p2x, ship->starsystem->p2y, ship->starsystem->p2z );
   if( ship->starsystem->planet3 && str_cmp( ship->starsystem->planet3, "" ) )
      ch_printf( ch, "%s   %d %d %d\r\n",
                 ship->starsystem->planet3, ship->starsystem->p3x, ship->starsystem->p3y, ship->starsystem->p3z );
   ch_printf( ch, "\r\n" );
   for( target = ship->starsystem->first_ship; target; target = target->next_in_starsystem )
   {
      if( target != ship )
         ch_printf( ch, "%s    %.0f %.0f %.0f\r\n", target->name, target->vx, target->vy, target->vz );
   }
   ch_printf( ch, "\r\n" );
   for( missile = ship->starsystem->first_missile; missile; missile = missile->next_in_starsystem )
   {
      ch_printf( ch, "%s    %d %d %d\r\n",
                 missile->missiletype == CONCUSSION_MISSILE ? "A Concusion missile" :
                 ( missile->missiletype == PROTON_TORPEDO ? "A Torpedo" :
                   ( missile->missiletype == HEAVY_ROCKET ? "A Heavy Rocket" : "A Heavy Bomb" ) ),
                 missile->mx, missile->my, missile->mz );
   }

   ch_printf( ch, "\r\n&WYour Coordinates: %.0f %.0f %.0f\r\n", ship->vx, ship->vy, ship->vz );


   learn_from_success( ch, gsn_navigation );

}

void do_autotrack( CHAR_DATA * ch, const char *argument )
{
   SHIP_DATA *ship;
   int schance;

   if( ( ship = ship_from_cockpit( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou must be in the cockpit of a ship to do that!\r\n", ch );
      return;
   }

   if( ship->ship_class > SHIP_PLATFORM )
   {
      send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
      return;
   }


   if( ship->ship_class == SHIP_PLATFORM )
   {
      send_to_char( "&RPlatforms don't have autotracking systems!\r\n", ch );
      return;
   }
   if( ship->ship_class == CAPITAL_SHIP )
   {
      send_to_char( "&RThis ship is too big for autotracking!\r\n", ch );
      return;
   }

   if( ( ship = ship_from_pilotseat( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou aren't in the pilots chair!\r\n", ch );
      return;
   }

   if( autofly( ship ) )
   {
      send_to_char( "&RYou'll have to turn off the ships autopilot first....\r\n", ch );
      return;
   }

   schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_shipsystems] );
   if( number_percent(  ) > schance )
   {
      send_to_char( "&RYour notsure which switch to flip.\r\n", ch );
      learn_from_failure( ch, gsn_shipsystems );
      return;
   }

   act( AT_PLAIN, "$n flips a switch on the control panel.", ch, NULL, argument, TO_ROOM );
   if( ship->autotrack )
   {
      ship->autotrack = FALSE;
      echo_to_cockpit( AT_YELLOW, ship, "Autotracking off." );
   }
   else
   {
      ship->autotrack = TRUE;
      echo_to_cockpit( AT_YELLOW, ship, "Autotracking on." );
   }

   learn_from_success( ch, gsn_shipsystems );

}

void do_jumpvector( CHAR_DATA * ch, const char *argument )
{
}
void do_reload( CHAR_DATA * ch, const char *argument )
{
}
void do_closebay( CHAR_DATA * ch, const char *argument )
{
}
void do_openbay( CHAR_DATA * ch, const char *argument )
{
}

void do_tractorbeam( CHAR_DATA * ch, const char *argument )
{
}

void do_pluogus( CHAR_DATA * ch, const char *argument )
{
   bool ch_comlink = FALSE;
   OBJ_DATA *obj;
   int next_planet, itt;

   for( obj = ch->last_carrying; obj; obj = obj->prev_content )
   {
      if( obj->pIndexData->item_type == ITEM_COMLINK )
         ch_comlink = TRUE;
   }

   if( !ch_comlink )
   {
      send_to_char( "You need a comlink to do that!\r\n", ch );
      return;
   }

   send_to_char( "Serin Pluogus Schedule Information:\r\n", ch );

   /*
    * current port 
    */

   if( bus_pos < 7 && bus_pos > 1 )
      ch_printf( ch, "The Pluogus is Currently docked at %s.\r\n", bus_stop[bus_planet] );

   /*
    * destinations 
    */

   next_planet = bus_planet;
   send_to_char( "Next stops: ", ch );

   if( bus_pos <= 1 )
      ch_printf( ch, "%s  ", bus_stop[next_planet] );

   for( itt = 0; itt < 3; itt++ )
   {
      next_planet++;
      if( next_planet >= MAX_BUS_STOP )
         next_planet = 0;
      ch_printf( ch, "%s  ", bus_stop[next_planet] );
   }

   ch_printf( ch, "\r\n\r\n" );

   send_to_char( "Serin Tocca Schedule Information:\r\n", ch );

   /*
    * current port 
    */

   if( bus_pos < 7 && bus_pos > 1 )
      ch_printf( ch, "The Tocca is Currently docked at %s.\r\n", bus_stop[bus2_planet] );

   /*
    * destinations 
    */

   next_planet = bus2_planet;
   send_to_char( "Next stops: ", ch );

   if( bus_pos <= 1 )
      ch_printf( ch, "%s  ", bus_stop[next_planet] );

   for( itt = 0; itt < 3; itt++ )
   {
      next_planet++;
      if( next_planet >= MAX_BUS_STOP )
         next_planet = 0;
      ch_printf( ch, "%s  ", bus_stop[next_planet] );
   }

   ch_printf( ch, "\r\n" );

}

void do_fly( CHAR_DATA * ch, const char *argument )
{
}

void do_drive( CHAR_DATA * ch, const char *argument )
{
   int dir;
   SHIP_DATA *ship;

   if( ( ship = ship_from_cockpit( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou must be in the drivers seat of a land vehicle to do that!\r\n", ch );
      return;
   }

   if( ship->ship_class < LAND_SPEEDER )
   {
      send_to_char( "&RThis isn't a land vehicle!\r\n", ch );
      return;
   }


   if( ship->shipstate == SHIP_DISABLED )
   {
      send_to_char( "&RThe drive is disabled.\r\n", ch );
      return;
   }

   if( ship->energy < 1 )
   {
      send_to_char( "&RTheres not enough fuel!\r\n", ch );
      return;
   }

   if( ( dir = get_door( argument ) ) == -1 )
   {
      send_to_char( "Usage: drive <direction>\r\n", ch );
      return;
   }

   drive_ship( ch, ship, get_exit( get_room_index( ship->location ), dir ), 0 );

}

ch_ret drive_ship( CHAR_DATA * ch, SHIP_DATA * ship, EXIT_DATA * pexit, int fall )
{
   ROOM_INDEX_DATA *in_room;
   ROOM_INDEX_DATA *to_room;
   ROOM_INDEX_DATA *from_room;
   ROOM_INDEX_DATA *original;
   char buf[MAX_STRING_LENGTH];
   const char *txt;
   const char *dtxt;
   ch_ret retcode;
   short door, distance;
   bool drunk = FALSE;
   CHAR_DATA *rch;
   CHAR_DATA *next_rch;


   if( !IS_NPC( ch ) )
      if( IS_DRUNK( ch, 2 ) && ( ch->position != POS_SHOVE ) && ( ch->position != POS_DRAG ) )
         drunk = TRUE;

   if( drunk && !fall )
   {
      door = number_door(  );
      pexit = get_exit( get_room_index( ship->location ), door );
   }

#ifdef DEBUG
   if( pexit )
   {
      sprintf( buf, "drive_ship: %s to door %d", ch->name, pexit->vdir );
      log_string( buf );
   }
#endif

   retcode = rNONE;
   txt = NULL;

   in_room = get_room_index( ship->location );
   from_room = in_room;
   if( !pexit || ( to_room = pexit->to_room ) == NULL )
   {
      if( drunk )
         send_to_char( "You drive into a wall in your drunken state.\r\n", ch );
      else
         send_to_char( "Alas, you cannot go that way.\r\n", ch );
      return rNONE;
   }

   door = pexit->vdir;
   distance = pexit->distance;

   if( IS_SET( pexit->exit_info, EX_WINDOW ) && !IS_SET( pexit->exit_info, EX_ISDOOR ) )
   {
      send_to_char( "Alas, you cannot go that way.\r\n", ch );
      return rNONE;
   }

   if( IS_SET( pexit->exit_info, EX_PORTAL ) && IS_NPC( ch ) )
   {
      act( AT_PLAIN, "Mobs can't use portals.", ch, NULL, NULL, TO_CHAR );
      return rNONE;
   }

   if( IS_SET( pexit->exit_info, EX_NOMOB ) && IS_NPC( ch ) )
   {
      act( AT_PLAIN, "Mobs can't enter there.", ch, NULL, NULL, TO_CHAR );
      return rNONE;
   }

   if( IS_SET( pexit->exit_info, EX_CLOSED ) && ( IS_SET( pexit->exit_info, EX_NOPASSDOOR ) ) )
   {
      if( !IS_SET( pexit->exit_info, EX_SECRET ) && !IS_SET( pexit->exit_info, EX_DIG ) )
      {
         if( drunk )
         {
            act( AT_PLAIN, "$n drives into the $d in $s drunken state.", ch, NULL, pexit->keyword, TO_ROOM );
            act( AT_PLAIN, "You drive into the $d in your drunken state.", ch, NULL, pexit->keyword, TO_CHAR );
         }
         else
            act( AT_PLAIN, "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
      }
      else
      {
         if( drunk )
            send_to_char( "You hit a wall in your drunken state.\r\n", ch );
         else
            send_to_char( "Alas, you cannot go that way.\r\n", ch );
      }

      return rNONE;
   }

   if( room_is_private( ch, to_room ) )
   {
      send_to_char( "That room is private right now.\r\n", ch );
      return rNONE;
   }

   if( !IS_IMMORTAL( ch ) && !IS_NPC( ch ) && ch->in_room->area != to_room->area )
   {
      if( ch->top_level < to_room->area->low_hard_range )
      {
         set_char_color( AT_TELL, ch );
         switch ( to_room->area->low_hard_range - ch->top_level )
         {
            case 1:
               send_to_char( "A voice in your mind says, 'You are nearly ready to go that way...'", ch );
               break;
            case 2:
               send_to_char( "A voice in your mind says, 'Soon you shall be ready to travel down this path... soon.'", ch );
               break;
            case 3:
               send_to_char( "A voice in your mind says, 'You are not ready to go down that path... yet.'.\r\n", ch );
               break;
            default:
               send_to_char( "A voice in your mind says, 'You are not ready to go down that path.'.\r\n", ch );
         }
         return rNONE;
      }
      else if( ch->top_level > to_room->area->hi_hard_range )
      {
         set_char_color( AT_TELL, ch );
         send_to_char( "A voice in your mind says, 'There is nothing more for you down that path.'", ch );
         return rNONE;
      }
   }

   if( !fall )
   {
      if( IS_SET( to_room->room_flags, ROOM_INDOORS )
          || IS_SET( to_room->room_flags, ROOM_SPACECRAFT ) || to_room->sector_type == SECT_INSIDE )
      {
         send_to_char( "You can't drive indoors!\r\n", ch );
         return rNONE;
      }

      if( IS_SET( to_room->room_flags, ROOM_NO_DRIVING ) )
      {
         send_to_char( "You can't take a vehicle through there!\r\n", ch );
         return rNONE;
      }

      if( in_room->sector_type == SECT_AIR || to_room->sector_type == SECT_AIR || IS_SET( pexit->exit_info, EX_FLY ) )
      {
         if( ship->ship_class > CLOUD_CAR )
         {
            send_to_char( "You'd need to fly to go there.\r\n", ch );
            return rNONE;
         }
      }

      if( in_room->sector_type == SECT_WATER_NOSWIM
          || to_room->sector_type == SECT_WATER_NOSWIM
          || to_room->sector_type == SECT_WATER_SWIM
          || to_room->sector_type == SECT_UNDERWATER || to_room->sector_type == SECT_OCEANFLOOR )
      {

         if( ship->ship_class != OCEAN_SHIP )
         {
            send_to_char( "You'd need a boat to go there.\r\n", ch );
            return rNONE;
         }

      }

      if( IS_SET( pexit->exit_info, EX_CLIMB ) )
      {

         if( ship->ship_class < CLOUD_CAR )
         {
            send_to_char( "You need to fly or climb to get up there.\r\n", ch );
            return rNONE;
         }
      }

   }

   if( to_room->tunnel > 0 )
   {
      CHAR_DATA *ctmp;
      int count = 0;

      for( ctmp = to_room->first_person; ctmp; ctmp = ctmp->next_in_room )
         if( ++count >= to_room->tunnel )
         {
            send_to_char( "There is no room for you in there.\r\n", ch );
            return rNONE;
         }
   }

   if( fall )
      txt = "falls";
   else if( !txt )
   {
      if( ship->ship_class < OCEAN_SHIP )
         txt = "fly";
      else if( ship->ship_class == OCEAN_SHIP )
      {
         txt = "float";
      }
      else if( ship->ship_class > OCEAN_SHIP )
      {
         txt = "drive";
      }
   }
   sprintf( buf, "$n %ss the vehicle $T.", txt );
   act( AT_ACTION, buf, ch, NULL, dir_name[door], TO_ROOM );
   sprintf( buf, "You %s the vehicle $T.", txt );
   act( AT_ACTION, buf, ch, NULL, dir_name[door], TO_CHAR );
   sprintf( buf, "%s %ss %s.", ship->name, txt, dir_name[door] );
   echo_to_room( AT_ACTION, get_room_index( ship->location ), buf );

   extract_ship( ship );
   ship_to_room( ship, to_room->vnum );

   ship->location = to_room->vnum;
   ship->lastdoc = ship->location;

   if( fall )
      txt = "falls";
   else if( ship->ship_class < OCEAN_SHIP )
      txt = "flys in";
   else if( ship->ship_class == OCEAN_SHIP )
   {
      txt = "floats in";
   }
   else if( ship->ship_class > OCEAN_SHIP )
   {
      txt = "drives in";
   }

   switch ( door )
   {
      default:
         dtxt = "somewhere";
         break;
      case 0:
         dtxt = "the south";
         break;
      case 1:
         dtxt = "the west";
         break;
      case 2:
         dtxt = "the north";
         break;
      case 3:
         dtxt = "the east";
         break;
      case 4:
         dtxt = "below";
         break;
      case 5:
         dtxt = "above";
         break;
      case 6:
         dtxt = "the south-west";
         break;
      case 7:
         dtxt = "the south-east";
         break;
      case 8:
         dtxt = "the north-west";
         break;
      case 9:
         dtxt = "the north-east";
         break;
   }

   sprintf( buf, "%s %s from %s.", ship->name, txt, dtxt );
   echo_to_room( AT_ACTION, get_room_index( ship->location ), buf );

   for( rch = ch->in_room->last_person; rch; rch = next_rch )
   {
      next_rch = rch->prev_in_room;
      original = rch->in_room;
      char_from_room( rch );
      char_to_room( rch, to_room );
      do_look( rch, "auto" );
      char_from_room( rch );
      char_to_room( rch, original );
   }

/*
    if (  CHECK FOR FALLING HERE
    &&   fall > 0 )
    {
	if (!IS_AFFECTED( ch, AFF_FLOATING )
	|| ( ch->mount && !IS_AFFECTED( ch->mount, AFF_FLOATING ) ) )
	{
	  set_char_color( AT_HURT, ch );
	  send_to_char( "OUCH! You hit the ground!\r\n", ch );
	  WAIT_STATE( ch, 20 );
	  retcode = damage( ch, ch, 50 * fall, TYPE_UNDEFINED );
	}
	else
	{
	  set_char_color( AT_MAGIC, ch );
	  send_to_char( "You lightly float down to the ground.\r\n", ch );
	}
    }

*/
   return retcode;

}

void do_bomb( CHAR_DATA * ch, const char *argument )
{
}

void do_chaff( CHAR_DATA * ch, const char *argument )
{
   int schance;
   SHIP_DATA *ship;


   if( ( ship = ship_from_cockpit( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RYou must be in the cockpit of a ship to do that!\r\n", ch );
      return;
   }

   if( ship->ship_class > SHIP_PLATFORM )
   {
      send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
      return;
   }


   if( ( ship = ship_from_coseat( ch->in_room->vnum ) ) == NULL )
   {
      send_to_char( "&RThe controls are at the copilots seat!\r\n", ch );
      return;
   }

   if( autofly( ship ) )
   {
      send_to_char( "&RYou'll have to turn the autopilot off first...\r\n", ch );
      return;
   }

   if( ship->shipstate == SHIP_HYPERSPACE )
   {
      send_to_char( "&RYou can only do that in realspace!\r\n", ch );
      return;
   }
   if( ship->shipstate == SHIP_DOCKED )
   {
      send_to_char( "&RYou can't do that until after you've launched!\r\n", ch );
      return;
   }
   if( ship->chaff <= 0 )
   {
      send_to_char( "&RYou don't have any chaff to release!\r\n", ch );
      return;
   }
   schance = IS_NPC( ch ) ? ch->top_level : ( int )( ch->pcdata->learned[gsn_weaponsystems] );
   if( number_percent(  ) > schance )
   {
      send_to_char( "&RYou can't figure out which switch it is.\r\n", ch );
      learn_from_failure( ch, gsn_weaponsystems );
      return;
   }

   ship->chaff--;

   ship->chaff_released++;

   send_to_char( "You flip the chaff release switch.\r\n", ch );
   act( AT_PLAIN, "$n flips a switch on the control pannel", ch, NULL, argument, TO_ROOM );
   echo_to_cockpit( AT_YELLOW, ship, "A burst of chaff is released from the ship." );

   learn_from_success( ch, gsn_weaponsystems );

}

bool autofly( SHIP_DATA * ship )
{

   if( !ship )
      return FALSE;

   if( ship->type == MOB_SHIP )
      return TRUE;

   if( ship->autopilot )
      return TRUE;

   return FALSE;

}



/* Generic Pilot Command To use as template

void do_hmm( CHAR_DATA *ch, const char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    int schance;
    SHIP_DATA *ship;
    
    strcpy( arg, argument );    
    
    switch( ch->substate )
    { 
    	default:
    	        if (  (ship = ship_from_cockpit(ch->in_room->vnum))  == NULL )
    	        {
    	            send_to_char("&RYou must be in the cockpit of a ship to do that!\r\n",ch);
    	            return;
    	        }
                if (ship->shipstate == SHIP_HYPERSPACE)
                {
                  send_to_char("&RYou can only do that in realspace!\r\n",ch);
                  return;   
                }
                if (ship->shipstate == SHIP_DISABLED)
    	        {
    	            send_to_char("&RThe ships drive is disabled. Unable to manuever.\r\n",ch);
    	            return;
    	        }
    	        if (ship->shipstate == SHIP_DOCKED)
    	        {
    	            send_to_char("&RYou can't do that until after you've launched!\r\n",ch);
    	            return;
    	        }
    	        if (ship->shipstate != SHIP_READY)
    	        {
    	            send_to_char("&RPlease wait until the ship has finished its current manouver.\r\n",ch);
    	            return;
    	        }
        
                if ( ship->energy <1 )
    	        {
    	           send_to_char("&RTheres not enough fuel!\r\n",ch);
    	           return;
    	        }
    	        
                if ( ship->ship_class == FIGHTER_SHIP )
                    schance = IS_NPC(ch) ? ch->top_level
	                 : (int)  (ch->pcdata->learned[gsn_starfighters]) ;
                if ( ship->ship_class == MIDSIZE_SHIP )
                    schance = IS_NPC(ch) ? ch->top_level
	                 : (int)  (ch->pcdata->learned[gsn_midships]) ;
                if ( ship->ship_class == CAPITAL_SHIP )
                    schance = IS_NPC(ch) ? ch->top_level
	                 : (int) (ch->pcdata->learned[gsn_capitalships]);
                if ( number_percent( ) < schance )
    		{
    		   send_to_char( "&G\r\n", ch);
    		   act( AT_PLAIN, "$n does  ...", ch,
		        NULL, argument , TO_ROOM );
		   echo_to_room( AT_YELLOW , get_room_index(ship->cockpit) , "");
    		   add_timer ( ch , TIMER_DO_FUN , 1 , do_hmm , 1 );
    		   ch->dest_buf = str_dup(arg);
    		   return;
	        }
	        send_to_char("&RYou fail to work the controls properly.\r\n",ch);
	        if ( ship->ship_class == FIGHTER_SHIP )
                    learn_from_failure( ch, gsn_starfighters );
                if ( ship->ship_class == MIDSIZE_SHIP )
    	            learn_from_failure( ch, gsn_midships );
                if ( ship->ship_class == CAPITAL_SHIP )
                    learn_from_failure( ch, gsn_capitalships );
    	   	return;	
    	
    	case 1:
    		if ( !ch->dest_buf )
    		   return;
    		strcpy(arg, ch->dest_buf);
    		DISPOSE( ch->dest_buf);
    		break;
    		
    	case SUB_TIMER_DO_ABORT:
    		DISPOSE( ch->dest_buf );
    		ch->substate = SUB_NONE;
    		if ( (ship = ship_from_cockpit(ch->in_room->vnum)) == NULL )
    		      return;    		                                   
    	        send_to_char("&Raborted.\r\n", ch);
    	        echo_to_room( AT_YELLOW , get_room_index(ship->cockpit) , "");
    		if (ship->shipstate != SHIP_DISABLED)
    		   ship->shipstate = SHIP_READY;
    		return;
    }
    
    ch->substate = SUB_NONE;
    
    if ( (ship = ship_from_cockpit(ch->in_room->vnum)) == NULL )
    {  
       return;
    }

    send_to_char( "&G\r\n", ch);
    act( AT_PLAIN, "$n does  ...", ch,
         NULL, argument , TO_ROOM );
    echo_to_room( AT_YELLOW , get_room_index(ship->cockpit) , "");

         
    if ( ship->ship_class == FIGHTER_SHIP )
        learn_from_success( ch, gsn_starfighters );
    if ( ship->ship_class == MIDSIZE_SHIP )
        learn_from_success( ch, gsn_midships );
    if ( ship->ship_class == CAPITAL_SHIP )
        learn_from_success( ch, gsn_capitalships );
    	
}

void do_hmm( CHAR_DATA *ch, const char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    int schance;
    SHIP_DATA *ship;
    
   
        if (  (ship = ship_from_cockpit(ch->in_room->vnum))  == NULL )
        {
            send_to_char("&RYou must be in the cockpit of a ship to do that!\r\n",ch);
            return;
        }
        
                if (ship->shipstate == SHIP_HYPERSPACE)
                {
                  send_to_char("&RYou can only do that in realspace!\r\n",ch);
                  return;   
                }
                if (ship->shipstate == SHIP_DISABLED)
    	        {
    	            send_to_char("&RThe ships drive is disabled. Unable to manuever.\r\n",ch);
    	            return;
    	        }
    	        if (ship->shipstate == SHIP_DOCKED)
    	        {
    	            send_to_char("&RYou can't do that until after you've launched!\r\n",ch);
    	            return;
    	        }
    	        if (ship->shipstate != SHIP_READY)
    	        {
    	            send_to_char("&RPlease wait until the ship has finished its current manouver.\r\n",ch);
    	            return;
    	        } 
        
        if ( ship->energy <1 )
        {
              send_to_char("&RTheres not enough fuel!\r\n",ch);
              return;
        }
    	        
        if ( ship->ship_class == FIGHTER_SHIP )
             schance = IS_NPC(ch) ? ch->top_level
             : (int)  (ch->pcdata->learned[gsn_starfighters]) ;
        if ( ship->ship_class == MIDSIZE_SHIP )
             schance = IS_NPC(ch) ? ch->top_level
                 : (int)  (ch->pcdata->learned[gsn_midships]) ;
        if ( ship->ship_class == CAPITAL_SHIP )
              schance = IS_NPC(ch) ? ch->top_level
                 : (int) (ch->pcdata->learned[gsn_capitalships]);
        if ( number_percent( ) > schance )
        {
            send_to_char("&RYou fail to work the controls properly.\r\n",ch);
            if ( ship->ship_class == FIGHTER_SHIP )
               learn_from_failure( ch, gsn_starfighters );
            if ( ship->ship_class == MIDSIZE_SHIP )   
               learn_from_failure( ch, gsn_midships );
            if ( ship->ship_class == CAPITAL_SHIP )
                learn_from_failure( ch, gsn_capitalships );
    	   return;	
        }
        
    send_to_char( "&G\r\n", ch);
    act( AT_PLAIN, "$n does  ...", ch,
         NULL, argument , TO_ROOM );
    echo_to_room( AT_YELLOW , get_room_index(ship->cockpit) , "");
	  
    
    
    if ( ship->ship_class == FIGHTER_SHIP )
        learn_from_success( ch, gsn_starfighters );
    if ( ship->ship_class == MIDSIZE_SHIP )
        learn_from_success( ch, gsn_midships );
    if ( ship->ship_class == CAPITAL_SHIP )
        learn_from_success( ch, gsn_capitalships );
    	
}

*/
