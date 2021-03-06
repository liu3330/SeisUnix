* Copyright (c) Colorado School of Mines, 1990.
* All rights reserved.

      subroutine setref(event,irefl,nrefls,ievent,valid,maxevt,
     :                  maxref,nint)

c     Given character input (event), this subroutine sets the list
c     of reflectors for this event and counts the number of reflections
c     that occur in this event.  Assumes that 2 digits are enough to 
c     specify a reflector (ie, 99 is max interface number)

      character    event*30
      logical      valid
      integer      nrefls,    ievent,    maxevt,   maxref,   nint

      integer   irefl(maxevt,0:maxref)

cc    Local variables:
c     DIGITS   The digits 0 - 9, and space
c     IDIG1    First digit of pair
c     IDIG2    Second digit of pair
c     J,K      Counters

      character  digits*11
      integer    idig1,    idig2,   j,   k


      digits = ' 0123456789'
      valid = .true.
      nrefls = 1

c     look at characters - max of 30
c     pass over blank spaces

      j = 1
10    if(j.le.30) then

         if(event(j:j).eq.digits(1:1)) then
c           this is a space
            j = j + 1
         else   
c           calculate the digit
            k = 2
20          if(event(j:j).ne.digits(k:k)) then
               k = k + 1
               if(k.gt.11) then
c                 character not 0 - 9
                  valid = .false.
                  return
               end if
               go to 20 
            end if
c           got the first digit of possible pair
            idig1 = k - 2
   
c           next character
            j = j + 1
            if(j.gt.30) then
c              only one digit
c              set reflector
               irefl(ievent,nrefls) = idig1
            else  
c              set number (maximum of two digits)
               if(event(j:j).eq.digits(1:1)) then
c                 space - only one digit
                  irefl(ievent,nrefls) = idig1
               else
                  k = 2
30                if(event(j:j).ne.digits(k:k)) then
                     k = k + 1
                     if(k.gt.11) then
                        valid = .false.
                        return
                     end if
                     go to 30
                  end if
c                 got  the second digit
                  idig2 = k - 2
c                 set reflector
                  irefl(ievent,nrefls) = 10 * idig1 + idig2
c                 make sure next character is a space (2 digits max)
                  if(event(j+1:j+1).ne.digits(1:1)) then
                     valid = .false.
                     return
                  end if
               end if

               j = j + 1
            end if
            nrefls = nrefls + 1

         end if
         go to 10 

      end if

c     number of reflections in this event
      nrefls = nrefls - 1

c     make sure these are valid reflector numbers
      do 50 j = 1,  nrefls
         if(irefl(ievent,j).gt.nint) then
            valid = .false.
            return
         end if
50       continue

      return 
      end

c-----------------------------------------------------------------------

      subroutine order(kref,nrefls,is,ievent,vel,norder,v,sign,
     : vref,n,valid,recdpt,list,raylst,stderr,maxevt,maxref)

c     Calculates the order in which the ray meets the interfaces.
c     Sets velocities on ray segments.
c     Sets direction in which a ray leaves an interface (through sign()).
c     Finds the velocity of the reflecting layers.

      real     vel(0:*),  v(*),  sign(0:*),  vref(*),  recdpt
      integer  ievent,    is,    raylst,     nrefls,   stderr,   n
      integer  kref(maxevt,0:maxref),        norder(*)
      logical  valid,     list

cc    Local variables:
c     DOWN     TRUE when ray is going down
c     I        Counter
c     IMAX     Max interface number at ends of ray segment
c     INC      +-1, depending on whether ray is going up or down
c     I1       Interface number of previous reflector
c     I2       Interface number of next reflector
c     ITRAK    Tracks the number of intersections
c     K,L      Counters
c     UP       TRUE when ray is going up

      integer  i,   imax,   inc,   i1,   i2,   itrak,  k,  l
      logical  up,  down


      valid = .true.
c     first set order of intersections

      itrak = 0
c     source layer
      kref(ievent,0) = is

      if(kref(ievent,1).ge.kref(ievent,0)) then
c        ray is going down from source
         norder(1) = kref(ievent,0)
         itrak = itrak + 1
         down = .true.
         up = .false.
      else
c        ray is going up from source
         down = .false.
         up = .true.
      end if

      do 100 l = 1,  nrefls
         if(kref(ievent,l).ge.kref(ievent,l-1).and.down) then
c           ray is going down
            inc = 1
            i1 = kref(ievent,l-1) + 1
            i2 = kref(ievent,l)
         else if(kref(ievent,l).lt.kref(ievent,l-1).and.up) then
c           ray is going up   
            inc = - 1
            i1 = kref(ievent,l-1)  - 1
            i2 = kref(ievent,l)
         else
            valid = .false.
            if(list) then
               write(raylst,'(2x,a)')
     :         'Invalid event.'
               write(raylst,'(2x,a)')
     :         'May be due to source location or the reflector list.'
            end if
            return
         end if
      
c        set order of intersections betweeen ray and interfaces
         do 50 k = i1, i2, inc
            itrak = itrak + 1
            norder(itrak) = k
50          continue

         if(down) then
            down = .false.
            up   = .true.
         else
            down = .true.
            up   = .false.
         end if

100      continue

      if(down.and.kref(ievent,nrefls).ne.0) then
c        last reflection sent ray downwards and was not from
c        the upper surface - therefore this must be an invalid
c        event description since ray never makes it to receivers
         valid = .false.     
         if(list) then
            write(raylst,'(2x,a)')
     :      'Invalid event.'
            write(raylst,'(2x,a)')
     :      'May be due to source location or the reflector list.'
         end if
         return
      end if

      if(kref(ievent,nrefls).eq.0.and.recdpt.le.0.) then
c        asked for a receiver ghost, but receivers are not
c        buried below the surface - so invalid event
         valid = .false.
         write(stderr,'(1x,a,1x,i2)')
     :   'Invalid event - number',ievent
         write(stderr,'(1x,a)')
     :   'You must bury the receivers to get a receiver ghost.'
         if(list) then
            write(raylst,'(1x,a)')
     :      'Invalid event.'
            write(raylst,'(1x,a)')
     :      'You must bury the receivers to get a receiver ghost.'
         end if
         return
      end if

c     extend ray back to upper surface (receivers are always in layer 1)
      if(kref(ievent,nrefls).eq.0) then
c        last reflection was from upper surface
      else
c        last reflection deep in model
         do 110 k = kref(ievent,nrefls) - 1, 1, -1
            itrak = itrak + 1
            norder(itrak) = k
110         continue
      end if

c     set n
      n = itrak

c     now set velocities on ray segments
      v(1) = vel(is)
      do 210 i = 2,  n
         imax = max(norder(i),norder(i-1))
         v(i) = vel(imax)
210      continue
c     last segment is always in layer 1
      v(n+1) = vel(1)
    
c     set sign() ie, direction of ray leaving an interface
      do 250 i = 1,  n - 1
         if(norder(i+1).gt.norder(i)) then
c           ray going down
            sign(i) = -1.
         else
c           ray going up
            sign(i) = 1.
         end if
250      continue

      if(norder(1).lt.is) then
c        ray goes up from source
         sign(0) = 1.
      else
c        ray goes down from source
         sign(0) = -1.
      end if

      if(norder(n).eq.1) then
c        last segment going up
         sign(n) = 1.
      else
c        last segment going down (receiver ghost)
         sign(n) = -1.
      end if

c     set velocities of reflecting layers - vref()
      do 300 i = 1,  nrefls - 1
         if(kref(ievent,i+1).gt.kref(ievent,i)) then
c           reflecting down
            vref(i) = vel(kref(ievent,i))
         else
c           reflecting up
            vref(i) = vel(kref(ievent,i)+1)
         end if
300      continue
      if(kref(ievent,nrefls).eq.0) then
c        ghost from upper surface
         vref(nrefls) = vel(0)
      else
         vref(nrefls) = vel(kref(ievent,nrefls)+1)
      end if

c     write(*,*)'s0',sign(0)
      do 500 i = 1, n
c        write(*,*)i,norder(i),sign(i),v(i),vref(i)
500      continue
c     write(*,*)'vn+1',v(n+1)

      return
      end

c---------------------------------------------------------------------

      subroutine gap(irec,ntr1b,ntrnb,ntr1f,ingap,ntrabs)

c     Checks to see if a receiver is located in the gap

      integer  irec,  ntr1b,  ntrnb,  ntr1f,  ntrabs
      logical  ingap

      ntrabs = irec + ntr1b - 1
      if(ntrabs.gt.ntrnb.and.ntrabs.lt.ntr1f) then
         ingap = .true.
      else
         ingap = .false.
      end if

      return
      end

c--------------------------------------------------------------

      subroutine brackt(xnp1,xrec,nrec,sgnoff,nxtrec,nohdwv)

c     Brackets emergence point of critical ray

      integer  nrec,  nxtrec
      real     xnp1,  xrec(nrec),  sgnoff
      logical  nohdwv

cc    Local variables:
c     I     Counter


      integer i


      nohdwv = .false.

      if(sgnoff.lt.0.) then
c        shot to right of receivers

         if(xnp1.lt.xrec(1)) then
c           no headwaves for this half of spread
            nohdwv = .true.
            return   
         else if(xnp1.ge.xrec(nrec)) then
c           try to get to last receiver 
            nxtrec = nrec
            return    
         else
c           ray emerges inside line, bracket below
         end if

      else

c        sgnoff > 0.
c        shot to left of receivers
         if(xnp1.lt.xrec(1)) then
c           try to get to first receiver
            nxtrec = 1
            return     
         else if(xnp1.gt.xrec(nrec)) then
c           no headwaves for this half of spread
            nohdwv = .true.
            return   
         else
c           ray emerges inside line, bracket below
         end if
      end if
            
c     bracketing ray when it emerges within line
      do 963 i = 1,  nrec
         if(xnp1.ge.xrec(i).and.xnp1.lt.xrec(i+1))then
            if(sgnoff.lt.0.) then
               nxtrec = i
            else
               nxtrec = i + 1
            end if 
            return   
         end if
963      continue

      return
      end

c--------------------------------------------------------------

      subroutine xzout(x,z,n,iunit)

c     Writes ray coordinates to listing file

      integer   n,       iunit
      real      x(0:n),  z(0:n)

cc    Local variables:
c     I     Counter

      integer i

      DO 10 I = 0,  N
         WRITE(iunit,'(2f10.2)') X(I),Z(I)
10       CONTINUE

      return
      end
c------------------------------------------------------

      subroutine xzsrc(s1,nsrc,dsrc,zwell,nwell,w0,w1,w2,w3,
     :                xsrc,zsrc,stderr)
c     Calculates the x-z coordinates of the source in the well.

      integer   nsrc,       nwell,      stderr

      real      s1,         dsrc,        zwell(0:nwell-1),
     :          w0(nwell),  w1(nwell),   w2(nwell),   w3(nwell),
     :          xsrc(nsrc), zsrc(nsrc)


cc    Local variables:
c     DELS    integration step size (arc length down the well)
c     DELZ    change in Z due to step DELS
c     DXDZ    slope of well (dx/dz)
c     ISI     lower limit of integration
c     ISF     upper limit of integration
c     J,K,L   loop variables
c     XTRAK   x-coordinate in well
c     ZTRAK   z-coordinate in well

      real     dels,  delz,  dxdz,  xtrak,  ztrak
      integer  isi,   isf,   j,      k,     l    
      parameter( dels = 1.)

      ztrak = zwell(0)
      
      
c     First find x,z-coordinates of sources in well
      do 300 k = 1,  nsrc

c        set limits of integration
         if(k.eq.1) then
            isi = 0
            isf = s1
         else
            isi = s1 + ( k - 2 ) * dsrc
            isf = isi + dsrc
         end if
c        integrate down the well to next source
         do 250 l = isi + 1,  isf

            if(ztrak.lt.zwell(0).or.
     :      ztrak.ge.zwell(nwell-1)) then
               write(stderr,'(a)')'XZREC: receivers outside well.'
               stop
            end if
            J = 1
210         IF(ztrak.ge.zwell(J-1)) THEN
               J = J + 1
               GO TO 210
            END IF
            J = J - 1
c           slope of well
            dxdz =       w1(j)
     :           +  2. * w2(j) * ztrak
     :           +  3. * w3(j) * ztrak**2
   
c           change in z brought about by dels increment in distance (arc
c           length) down well
            delz = dels / sqrt( 1. + dxdz**2 )
c           z coordinate at this distance down the well
            ztrak = ztrak + delz

250         continue

c           find x-coordinate at this source
            J = 1
260         IF(ztrak.ge.zwell(J-1)) THEN
               J = J + 1
               GO TO 260
            END IF
            J = J - 1
            xtrak = w0(j) + w1(j) * ztrak
     :                    + w2(j) * ztrak**2
     :                    + w3(j) * ztrak**3

c           set x,z-coordinates of source
            xsrc(k) = xtrak
            zsrc(k) = ztrak
 
300         continue

         return
         end

c----------------------------------------------------------------------

      subroutine setvar(p,logic1,logic2,logic3,logic4,
     :                  c1,c1cap,c2,c2cap,c3,c3cap)
c     Sets logical variables given character input.
      
      character p*3, c1*1, c2*1, c3*1, c1cap*1, c2cap*1, c3cap*1

      logical logic1, logic2, logic3, logic4
   
cc    Local variables:
c     J   counter
      integer  j

      logic1 = .false.
      logic2 = .false.
      logic3 = .false.

      do 200 j = 1,  3
         if(p(j:j).eq.c1.or.p(j:j).eq.c1cap) then
            logic1 = .true.
            logic4 = .true.
         else if(p(j:j).eq.c2.or.p(j:j).eq.c2cap) then
            logic2 = .true.
            logic4 = .true.
         else if(p(j:j).eq.c3.or.p(j:j).eq.c3cap) then
            logic3 = .true.
         end if
200      continue

         return
         end

c----------------------------------------------------------------------

      subroutine layer(x,z,slayer,inside,
     1                 XINT,ZINT,A0,A1,A2,A3,SIGN,NPTS,NINT,NORDER,
     2         	       MAXINT,MAXSPL,MAXN,MXSPM1)

c     Given coordinates of source point, computes layer in which
c     source is located

      real       x,            z
      integer    slayer
      logical    inside

      INTEGER    MAXINT,       MAXSPL,         MXSPM1,
     :           MAXN

      REAL        XINT(0:MAXINT,MAXSPL),     ZINT(0:MAXINT,MAXSPL)
      REAL*8      A0(0:MAXINT,MXSPM1),       A1(0:MAXINT,MXSPM1),
     :            A2(0:MAXINT,MXSPM1),       A3(0:MAXINT,MXSPM1)
      REAL        SIGN(0:MAXN)

      INTEGER     NPTS(0:MAXINT),   NINT,      NORDER(MAXN)

cc    Local variables:
c     IINT    interface counter
c     J       counter
c     ZINTK   depth of interface IINT at x-coordinate of source

      integer  iint,  j
      real     zintk


      inside = .true.

      iint = 1
5     if(x.le.xint(iint,1).or.x.gt.xint(iint,npts(iint))) then
c        source outside model
         inside = .false.
         return
      end if

c     find depth of interface at this source location
      J = 1
10    IF(x.gt.XINT(iint,J)) THEN
         J = J + 1
         GO TO 10
      END IF
      J = J - 1
c     interface depth...
      zintk = a0(iint,j) + a1(iint,j) * x
     :                   + a2(iint,j) * x**2
     :                   + a3(iint,j) * x**3

      if(zintk.gt.z) then
c        source above interface
         slayer = iint
      else
c        source below interface
         iint = iint + 1
         if(iint.gt.nint) then
c           source below last interface
            slayer = nint + 1
         else
c           next interface
            go to 5
         end if
      end if

      return
      end

c----------------------------------------------------------------------

      subroutine elevs(xrec,nrec,recdpt,zrec,splnok,
     1                 XINT,ZINT,A0,A1,A2,A3,SIGN,NPTS,NINT,NORDER,
     2         	       MAXINT,MAXSPL,MAXN,MXSPM1)

c     Given the x-coordinates, finds the z-coordinates (elevations less
c     station depth) of NREC stations

      integer    nrec
      real       xrec(nrec),   recdpt,         zrec(nrec)
      logical    splnok

      INTEGER    MAXINT,       MAXSPL,         MXSPM1,
     :           MAXN

      REAL        XINT(0:MAXINT,MAXSPL),     ZINT(0:MAXINT,MAXSPL)
      REAL*8      A0(0:MAXINT,MXSPM1),       A1(0:MAXINT,MXSPM1),
     :            A2(0:MAXINT,MXSPM1),       A3(0:MAXINT,MXSPM1)
      REAL        SIGN(0:MAXN)

      INTEGER     NPTS(0:MAXINT),   NINT,      NORDER(MAXN)

cc    Local variables:
c     I,J   counters
c     X     x-coordinate of station
c     Z     elevation of station

      real         x,    z
      integer      i,    j

      splnok = .true.

      DO 10 I = 1,  NREC
         x = xrec(i)
c        finding the section of the spline on which x lies and evaluating
c        the function.
c        if x falls outside the range of definition of the
c        upper surface, then return.
         IF(X.LE.XINT(0,1).OR.X.GT.XINT(0,NPTS(0))) THEN
            SPLNOK = .FALSE.
            RETURN
         END IF

         J = 1
5        IF(X.GT.XINT(0,J)) THEN
            J = J + 1
            GO TO 5
         END IF
         J = J - 1
         Z = A0(0,J) + A1(0,J) * X + A2(0,J) * X**2
     $          + A3(0,J) * X**3

c        z-coordinate is elevation less station depth
         zrec(i) = z + recdpt

10       CONTINUE

      return
      end

c-----------------------------------------------------------------------

      subroutine chkcrd(a,nnum)
c     Counts how many numbers have been specified in the record, up to 
c     the first noninteger character (excluding blank spaces, 
c     commas and decimal points)

      character a*81
      integer nnum

c     Local variables
c     DIGITS   character containing the digits and decimal point
c     I, J     counters

      character digits*15
      integer i, j

      digits = '1234567890.+-Ee'

      a(81:81) = ' '
      nnum = 0
      j = 1
      i = 1

50    if(j.le.80.and.i.le.15) then

         if(a(j:j).eq.' '.or.a(j:j).eq.',') then
c           this is a space or comma
            j = j + 1

         else if(a(j:j).eq.digits(i:i)) then
c           a digit or decimal point
            i = 1
            j = j + 1
            if(a(j:j).eq.' '.or.a(j:j).eq.',') then
c              end of number
               nnum = nnum + 1
            end if

         else
            i = i + 1

         end if

         go to 50

      end if

      return
      end
c-----------------------------------------------------------------
