/**************************************************************************
 *  Spivak Karaoke PLayer - a free, cross-platform desktop karaoke player *
 *  Copyright (C) 2015-2016 George Yunaev, support@ulduzsoft.com          *
 *                                                                        *
 *  This program is free software: you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation, either version 3 of the License, or     *
 *  (at your option) any later version.                                   *
 *																	      *
 *  This program is distributed in the hope that it will be useful,       *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *  GNU General Public License for more details.                          *
 *                                                                        *
 *  You should have received a copy of the GNU General Public License     *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 **************************************************************************/

#ifndef PLAYERBUTTON_H
#define PLAYERBUTTON_H

#include <QPixmap>
#include <QPushButton>

// Creates Amarok-like buttons
class PlayerButton : public QPushButton
{
	Q_OBJECT

	public:
		PlayerButton( QWidget *parent = 0 );
		void setPixmap( const QPixmap& icon );

	protected:
		void	paintEvent( QPaintEvent *event );
		QSize	minimumSizeHint() const;
		QSize	sizeHint() const;
		QSizePolicy sizePolicy () const;
        void 	enterEvent(QEnterEvent *e);
		void	leaveEvent(QEvent * e);

	private:
		QPoint	getAlignedCoords( const QPixmap& pix );

		bool		m_inButton;
		QPixmap		m_iconNormal;
		QPixmap		m_iconRaised;
		QPixmap		m_iconDown;

		// Where to draw icons so they're centered
		QPoint		m_pointNormal;
		QPoint		m_pointDown;
};

#endif // PLAYERBUTTON_H
